#include "services/interfaces/workflow/quake3/q3_bot_model_render.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace sdl3cpp::services::impl {
namespace {

// Build a column-major glm matrix from a tag stored in engine Y-up coords.
// An MD3 tag stores axis[i] as the child's basis vectors already
// expressed in the parent's space, so ioq3 attaches with
// VectorMA(origin, lerped.origin[i], parent->axis[i], origin)
// (cg_ents.c CG_PositionRotatedEntityOnTag): the i-th axis scales the
// i-th component, which makes each axis a column. Transposing here
// applies the inverse rotation, which twists the torso and head off the
// legs instead of following them.
glm::mat4 TagMatrix(const nlohmann::json& tag) {
    const auto& ax = tag["axis"];
    const auto& o  = tag["origin"];
    auto axis = [&](int i) {
        return glm::vec4(ax[i][0].get<float>(), ax[i][1].get<float>(),
                         ax[i][2].get<float>(), 0.0f);
    };
    return glm::mat4(
        axis(0), axis(1), axis(2),
        glm::vec4(o[0].get<float>(), o[1].get<float>(),
                  o[2].get<float>(), 1.0f)
    );
}

// Draws all surfaces of one MD3 model part at world transform `modelMat`.
void DrawBotModelPart(const std::string& prefix, int frame,
                      const glm::mat4& modelMat, const glm::mat4& view,
                      const glm::mat4& proj, const glm::vec3& camPos,
                      const glm::mat4& shadowVP,
                      const rendering::FragmentUniformData& fu,
                      SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                      SDL_GPUTexture* shadowTex, SDL_GPUSampler* shadowSamp,
                      WorkflowContext& context) {
    const int nSurfs = context.Get<int>("q3.md3." + prefix + "_num_surfs", 0);
    if (nSurfs <= 0) return;
    const int nFrames = context.Get<int>("q3.md3." + prefix + "_num_frames", 1);
    const int clampedFrame = std::max(0, std::min(frame, nFrames - 1));

    const glm::mat4 mvp = proj * view * modelMat;
    rendering::VertexUniformData vu = {};
    std::memcpy(vu.mvp,       glm::value_ptr(mvp),      sizeof(float) * 16);
    std::memcpy(vu.model_mat, glm::value_ptr(modelMat), sizeof(float) * 16);
    vu.normal[1] = 1.0f;
    vu.uv_scale[0] = 1.0f; vu.uv_scale[1] = 1.0f;
    vu.camera_pos[0] = camPos.x;
    vu.camera_pos[1] = camPos.y;
    vu.camera_pos[2] = camPos.z;
    std::memcpy(vu.shadow_vp, glm::value_ptr(shadowVP), sizeof(float) * 16);

    for (int s = 0; s < nSurfs; ++s) {
        const std::string sk = "q3.md3." + prefix + "_surf" + std::to_string(s);
        auto* vb = context.Get<SDL_GPUBuffer*>(
            sk + "_f" + std::to_string(clampedFrame) + "_vb", nullptr);
        auto* ib = context.Get<SDL_GPUBuffer*>(sk + "_ib", nullptr);
        const int numIdx = context.Get<int>(sk + "_num_idx", 0);
        if (!vb || !ib || numIdx <= 0) continue;
        auto* tex  = context.Get<SDL_GPUTexture*>(sk + "_tex",  nullptr);
        auto* samp = context.Get<SDL_GPUSampler*>(sk + "_samp", nullptr);
        if (!tex || !samp) continue;

        // Always bind 2 samplers — shader slot 1 is the shadow map.
        // Reuse albedo when no shadow texture to avoid Metal null-slot
        // validation errors.
        {
            auto* stex  = shadowTex  ? shadowTex  : tex;
            auto* ssamp = shadowSamp ? shadowSamp : samp;
            SDL_GPUTextureSamplerBinding b[2] = {{tex, samp}, {stex, ssamp}};
            SDL_BindGPUFragmentSamplers(pass, 0, b, 2);
        }
        SDL_GPUBufferBinding vbb = {vb, 0};
        SDL_BindGPUVertexBuffers(pass, 0, &vbb, 1);
        SDL_GPUBufferBinding ibb = {ib, 0};
        SDL_BindGPUIndexBuffer(pass, &ibb, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
        SDL_PushGPUFragmentUniformData(cmd, 0, &fu, sizeof(fu));
        SDL_DrawGPUIndexedPrimitives(pass, (uint32_t)numIdx, 1, 0, 0, 0);
    }
}

// Looks up a named attachment tag for `prefix`'s given frame, returning
// identity if the MD3/frame/tag is absent.
glm::mat4 GetBotModelTagMatrix(const std::string& prefix, int frame,
                               const std::string& tagName,
                               WorkflowContext& context) {
    const auto* tagsJson =
        context.TryGet<nlohmann::json>("q3.md3." + prefix + "_tags");
    if (!tagsJson || !tagsJson->is_array() ||
        (int)tagsJson->size() <= frame) {
        return glm::mat4(1.0f);
    }
    const auto& frameObj = (*tagsJson)[(size_t)frame];
    if (!frameObj.contains(tagName)) return glm::mat4(1.0f);
    return TagMatrix(frameObj[tagName]);
}

}  // namespace

void DrawBotModelChain(const nlohmann::json& bot,
                       const BotModelPrefixes& prefixes,
                       const glm::mat4& view, const glm::mat4& proj,
                       const glm::vec3& camPos, const glm::mat4& shadowVP,
                       const rendering::FragmentUniformData& fu,
                       SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                       SDL_GPUTexture* shadowTex, SDL_GPUSampler* shadowSamp,
                       WorkflowContext& context) {
    if (bot.value("state", std::string{}) == "dead") return;

    const auto& posJ = bot["pos"];
    const glm::vec3 bpos(posJ[0].get<float>(), posJ[1].get<float>(),
                          posJ[2].get<float>());
    const float yaw       = bot.value("yaw", 0.0f);
    const int legFrame    = bot.value("leg_frame", 0);
    const int torsoFrame  = bot.value("torso_frame", 0);

    auto draw = [&](const std::string& pfx, int frame,
                    const glm::mat4& modelMat) {
        DrawBotModelPart(pfx, frame, modelMat, view, proj, camPos, shadowVP,
                         fu, pass, cmd, shadowTex, shadowSamp, context);
    };

    // ── lower.md3: root transform ──────────────────────────────────
    // MD3 is Quake Z-up (X forward, Y left, Z up); a bare yaw about
    // world Y would leave the model on its side. Same remap as
    // q3.md3.draw, matching ioq3's AnglesToAxis().
    const glm::vec3 bf(-std::sin(yaw), 0.0f, -std::cos(yaw));
    const glm::vec3 bu(0.0f, 1.0f, 0.0f);
    glm::mat4 bOrient(1.0f);
    bOrient[0] = glm::vec4(bf, 0.0f);
    bOrient[1] = glm::vec4(glm::cross(bu, bf), 0.0f);
    bOrient[2] = glm::vec4(bu, 0.0f);
    const glm::mat4 lowerMat =
        glm::translate(glm::mat4(1.0f), bpos) * bOrient;
    draw(prefixes.lower, legFrame, lowerMat);
    if (!prefixes.hasUpper) return;

    // upper.md3: attached at tag_torso from lower.
    const glm::mat4 upperMat =
        lowerMat *
        GetBotModelTagMatrix(prefixes.lower, legFrame, "tag_torso", context);
    draw(prefixes.upper, torsoFrame, upperMat);
    if (!prefixes.hasHead) return;

    // head.md3: attached at tag_head from upper.
    draw(prefixes.head, 0,
         upperMat * GetBotModelTagMatrix(prefixes.upper, torsoFrame,
                                          "tag_head", context));
    if (!prefixes.hasWeapon) return;

    // weapon.md3: attached at tag_weapon from upper.
    draw(prefixes.weapon, 0,
         upperMat * GetBotModelTagMatrix(prefixes.upper, torsoFrame,
                                          "tag_weapon", context));
}

}  // namespace sdl3cpp::services::impl
