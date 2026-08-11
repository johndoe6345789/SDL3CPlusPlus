#include "services/interfaces/workflow/quake3/workflow_q3_bots_draw_step.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
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
// Tags are stored with axis[i] as the i-th row of the Q3 rotation matrix.
// In column-major (glm), we must transpose: column j = (axis[0][j], axis[1][j], axis[2][j]).
glm::mat4 TagMatrix(const nlohmann::json& tag) {
    const auto& ax = tag["axis"];
    const auto& o  = tag["origin"];
    return glm::mat4(
        glm::vec4(ax[0][0].get<float>(), ax[1][0].get<float>(), ax[2][0].get<float>(), 0.0f),
        glm::vec4(ax[0][1].get<float>(), ax[1][1].get<float>(), ax[2][1].get<float>(), 0.0f),
        glm::vec4(ax[0][2].get<float>(), ax[1][2].get<float>(), ax[2][2].get<float>(), 0.0f),
        glm::vec4(o[0].get<float>(),     o[1].get<float>(),     o[2].get<float>(),     1.0f)
    );
}

// Draw all surfaces of a named MD3 model at the given world transform.
void DrawMd3(const std::string& prefix, int frame, const glm::mat4& modelMat,
             const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos,
             const glm::mat4& shadowVP, const rendering::FragmentUniformData& fu,
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
    vu.camera_pos[0] = camPos.x; vu.camera_pos[1] = camPos.y; vu.camera_pos[2] = camPos.z;
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
        // Reuse albedo when no shadow texture to avoid Metal null-slot validation errors.
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

// Get a tag matrix from the context for a given MD3 prefix + frame + tag name.
// Returns identity if the tag is not found.
glm::mat4 GetTagMatrix(const std::string& prefix, int frame, const std::string& tagName,
                       WorkflowContext& context) {
    const auto* tagsJson = context.TryGet<nlohmann::json>("q3.md3." + prefix + "_tags");
    if (!tagsJson || !tagsJson->is_array() || (int)tagsJson->size() <= frame) return glm::mat4(1.0f);
    const auto& frameObj = (*tagsJson)[(size_t)frame];
    if (!frameObj.contains(tagName)) return glm::mat4(1.0f);
    return TagMatrix(frameObj[tagName]);
}

}  // namespace

WorkflowQ3BotsDrawStep::WorkflowQ3BotsDrawStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3BotsDrawStep::GetPluginId() const { return "q3.bots.draw"; }

void WorkflowQ3BotsDrawStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    auto* pass     = context.Get<SDL_GPURenderPass*>("gpu_render_pass",           nullptr);
    auto* cmd      = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer",     nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_textured", nullptr);
    const auto* botsPtr = context.TryGet<nlohmann::json>("q3.bots");
    if (!pass || !cmd || !pipeline || !botsPtr || !botsPtr->is_array()) return;

    WorkflowStepParameterResolver params;
    auto getStr = [&](const char* k, const std::string& def) -> std::string {
        const auto* p = params.FindParameter(step, k);
        return (p && p->type == WorkflowParameterValue::Type::String) ? p->stringValue : def;
    };

    const std::string lowerPfx  = getStr("lower_prefix",  "lower");
    const std::string upperPfx  = getStr("upper_prefix",  "upper");
    const std::string headPfx   = getStr("head_prefix",   "head");
    const std::string weaponPfx = getStr("weapon_prefix", "weapon_mg");

    const bool hasLower  = context.Get<int>("q3.md3." + lowerPfx + "_num_frames", 0) > 0;
    const bool hasUpper  = context.Get<int>("q3.md3." + upperPfx + "_num_frames", 0) > 0;
    const bool hasHead   = context.Get<int>("q3.md3." + headPfx  + "_num_frames", 0) > 0;
    const bool hasWeapon = context.Get<int>("q3.md3." + weaponPfx + "_num_frames", 0) > 0;
    if (!hasLower) return;  // nothing to draw without lower

    auto view     = context.Get<glm::mat4>("render.view_matrix",  glm::mat4(1.0f));
    auto proj     = context.Get<glm::mat4>("render.proj_matrix",  glm::mat4(1.0f));
    auto camPos   = context.Get<glm::vec3>("render.camera_pos",   glm::vec3(0.0f));
    auto shadowVP = context.Get<glm::mat4>("render.shadow_vp",    glm::mat4(1.0f));
    auto fu = context.Get<rendering::FragmentUniformData>("render.frag_uniforms",
                                                          rendering::FragmentUniformData{});
    fu.material[0] = 0.6f; fu.material[1] = 0.1f;  // roughness/metallic

    auto* shadowTex  = context.Get<SDL_GPUTexture*>("shadow_depth_texture", nullptr);
    auto* shadowSamp = context.Get<SDL_GPUSampler*>("shadow_depth_sampler", nullptr);

    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    for (const auto& bot : *botsPtr) {
        if (bot.value("state", std::string{}) == "dead") continue;

        const auto& posJ = bot["pos"];
        const glm::vec3 bpos(posJ[0].get<float>(), posJ[1].get<float>(), posJ[2].get<float>());
        const float yaw = bot.value("yaw", 0.0f);
        const int legFrame   = bot.value("leg_frame",   0);
        const int torsoFrame = bot.value("torso_frame", 0);

        // ── lower.md3: root transform ────────────────────────────────────────
        const glm::mat4 lowerMat =
            glm::translate(glm::mat4(1.0f), bpos)
            * glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0.0f, 1.0f, 0.0f));

        DrawMd3(lowerPfx, legFrame, lowerMat,
                view, proj, camPos, shadowVP, fu,
                pass, cmd, shadowTex, shadowSamp, context);

        if (!hasUpper) continue;

        // ── upper.md3: attached at tag_torso from lower ───────────────────────
        const glm::mat4 tagTorso   = GetTagMatrix(lowerPfx, legFrame, "tag_torso", context);
        const glm::mat4 upperMat   = lowerMat * tagTorso;

        DrawMd3(upperPfx, torsoFrame, upperMat,
                view, proj, camPos, shadowVP, fu,
                pass, cmd, shadowTex, shadowSamp, context);

        if (!hasHead) continue;

        // ── head.md3: attached at tag_head from upper ─────────────────────────
        const glm::mat4 tagHead  = GetTagMatrix(upperPfx, torsoFrame, "tag_head",  context);
        const glm::mat4 headMat  = upperMat * tagHead;

        DrawMd3(headPfx, 0, headMat,
                view, proj, camPos, shadowVP, fu,
                pass, cmd, shadowTex, shadowSamp, context);

        if (!hasWeapon) continue;

        // ── weapon.md3: attached at tag_weapon from upper ─────────────────────
        const glm::mat4 tagWeapon  = GetTagMatrix(upperPfx, torsoFrame, "tag_weapon", context);
        const glm::mat4 weaponMat  = upperMat * tagWeapon;

        DrawMd3(weaponPfx, 0, weaponMat,
                view, proj, camPos, shadowVP, fu,
                pass, cmd, shadowTex, shadowSamp, context);
    }
}

}  // namespace sdl3cpp::services::impl
