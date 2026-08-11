#include "services/interfaces/workflow/quake3/workflow_q3_md3_draw_step.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstring>
#include <string>

namespace sdl3cpp::services::impl {

namespace {

// Build a glm column-major matrix from a Q3/MD3 tag stored in engine Y-up coords.
// The tag axis[i] rows (already coord-converted) must be transposed to form
// the columns of a column-major matrix.
glm::mat4 TagMatrix(const nlohmann::json& tag) {
    const auto& ax = tag["axis"];  // 3 rows in engine space
    const auto& o  = tag["origin"];
    // Transpose: row i of Q3 axis → column i of glm matrix
    return glm::mat4(
        glm::vec4(ax[0][0].get<float>(), ax[1][0].get<float>(), ax[2][0].get<float>(), 0.0f),
        glm::vec4(ax[0][1].get<float>(), ax[1][1].get<float>(), ax[2][1].get<float>(), 0.0f),
        glm::vec4(ax[0][2].get<float>(), ax[1][2].get<float>(), ax[2][2].get<float>(), 0.0f),
        glm::vec4(o[0].get<float>(),     o[1].get<float>(),     o[2].get<float>(),     1.0f)
    );
}

// Draw all surfaces of a named MD3 at the given model matrix.
void DrawMd3Surfaces(const std::string& prefix, int frame,
                     const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj,
                     const glm::vec3& camPos, const glm::mat4& shadowVP,
                     const rendering::FragmentUniformData& fu,
                     SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                     SDL_GPUTexture* shadowTex, SDL_GPUSampler* shadowSamp,
                     WorkflowContext& context) {
    const int nSurfs = context.Get<int>("q3.md3." + prefix + "_num_surfs", 0);
    if (nSurfs <= 0) return;

    const glm::mat4 mvp = proj * view * model;
    rendering::VertexUniformData vu = {};
    std::memcpy(vu.mvp,       glm::value_ptr(mvp),     sizeof(float) * 16);
    std::memcpy(vu.model_mat, glm::value_ptr(model),   sizeof(float) * 16);
    vu.normal[1]    = 1.0f;
    vu.uv_scale[0]  = 1.0f; vu.uv_scale[1] = 1.0f;
    vu.camera_pos[0] = camPos.x; vu.camera_pos[1] = camPos.y; vu.camera_pos[2] = camPos.z;
    std::memcpy(vu.shadow_vp, glm::value_ptr(shadowVP), sizeof(float) * 16);

    for (int s = 0; s < nSurfs; ++s) {
        const std::string sk = "q3.md3." + prefix + "_surf" + std::to_string(s);
        auto* vb = context.Get<SDL_GPUBuffer*>(sk + "_f" + std::to_string(frame) + "_vb", nullptr);
        auto* ib = context.Get<SDL_GPUBuffer*>(sk + "_ib", nullptr);
        const int numIdx = context.Get<int>(sk + "_num_idx", 0);
        if (!vb || !ib || numIdx <= 0) continue;
        auto* tex  = context.Get<SDL_GPUTexture*>(sk + "_tex",  nullptr);
        auto* samp = context.Get<SDL_GPUSampler*>(sk + "_samp", nullptr);
        if (!tex || !samp) continue;

        // Always bind 2 samplers — shader slot 1 is the shadow map.
        // When no shadow texture is available, reuse the albedo texture so slot 1
        // is never null (Metal validation error). The shadow UV will be out of bounds
        // (shadow_vp is identity) so ComputeShadowPCF returns 1.0 (no shadow).
        {
            auto* stex  = shadowTex  ? shadowTex  : tex;
            auto* ssamp = shadowSamp ? shadowSamp : samp;
            SDL_GPUTextureSamplerBinding b[2] = {{tex, samp}, {stex, ssamp}};
            SDL_BindGPUFragmentSamplers(pass, 0, b, 2);
        }
        SDL_GPUBufferBinding vbBind = {vb, 0};
        SDL_BindGPUVertexBuffers(pass, 0, &vbBind, 1);
        SDL_GPUBufferBinding ibBind = {ib, 0};
        SDL_BindGPUIndexBuffer(pass, &ibBind, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
        SDL_PushGPUFragmentUniformData(cmd, 0, &fu, sizeof(fu));
        SDL_DrawGPUIndexedPrimitives(pass, (uint32_t)numIdx, 1, 0, 0, 0);
    }
}

}  // namespace

WorkflowQ3Md3DrawStep::WorkflowQ3Md3DrawStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3Md3DrawStep::GetPluginId() const { return "q3.md3.draw"; }

void WorkflowQ3Md3DrawStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    WorkflowStepParameterResolver params;
    auto getStr = [&](const char* k, const std::string& def) -> std::string {
        const auto* p = params.FindParameter(step, k);
        return (p && p->type == WorkflowParameterValue::Type::String) ? p->stringValue : def;
    };
    auto getNum = [&](const char* k, float def) -> float {
        const auto* p = params.FindParameter(step, k);
        return (p && p->type == WorkflowParameterValue::Type::Number) ? (float)p->numberValue : def;
    };
    auto getBool = [&](const char* k, bool def) -> bool {
        const auto* p = params.FindParameter(step, k);
        return (p && p->type == WorkflowParameterValue::Type::Bool) ? p->boolValue : def;
    };

    const std::string prefix    = getStr("prefix",    "model");
    const std::string pos_key   = getStr("pos_key",   "");
    const std::string yaw_key   = getStr("yaw_key",   "");
    const std::string frame_key = getStr("frame_key", "");
    const float fps        = getNum("fps",        15.0f);
    const int   animFirst  = (int)getNum("anim_first", 0.0f);
    const int   animCount  = (int)getNum("anim_count", 0.0f);
    const bool  viewmodel  = getBool("viewmodel", false);
    const float vmRight    = getNum("vm_right",  0.35f);
    const float vmDown     = getNum("vm_down",  -0.3f);
    const float vmFwd      = getNum("vm_fwd",    0.5f);

    auto* pass     = context.Get<SDL_GPURenderPass*>("gpu_render_pass",         nullptr);
    auto* cmd      = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer",   nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_textured", nullptr);
    if (!pass || !cmd || !pipeline) return;

    const int nFrames = context.Get<int>("q3.md3." + prefix + "_num_frames", 0);
    if (nFrames <= 0) return;

    // ── Determine animation frame ─────────────────────────────────────────────
    int frame = 0;
    if (!frame_key.empty()) {
        frame = context.Get<int>(frame_key, 0);
    } else {
        const double elapsed = context.GetDouble("frame.elapsed", 0.0);
        const int totalFrame = (int)(elapsed * fps);
        frame = (animCount > 0)
            ? animFirst + (totalFrame % animCount)
            : (totalFrame % nFrames);
    }
    frame = std::max(0, std::min(frame, nFrames - 1));

    // ── Build world model matrix ──────────────────────────────────────────────
    auto view     = context.Get<glm::mat4>("render.view_matrix",  glm::mat4(1.0f));
    auto proj     = context.Get<glm::mat4>("render.proj_matrix",  glm::mat4(1.0f));
    auto camPos   = context.Get<glm::vec3>("render.camera_pos",   glm::vec3(0.0f));
    auto shadowVP = context.Get<glm::mat4>("render.shadow_vp",    glm::mat4(1.0f));
    auto fu = context.Get<rendering::FragmentUniformData>("render.frag_uniforms",
                                                          rendering::FragmentUniformData{});

    glm::mat4 model(1.0f);
    if (viewmodel) {
        glm::vec3 right  (view[0][0], view[1][0], view[2][0]);
        glm::vec3 up     (view[0][1], view[1][1], view[2][1]);
        glm::vec3 forward(-view[0][2],-view[1][2],-view[2][2]);
        glm::vec3 pos = camPos + right * vmRight + up * vmDown + forward * vmFwd;
        // MD3 weapon models have barrel along local +X.
        // Map: model X → world forward (barrel points into screen)
        //      model Y → world up
        //      model Z → world right  (right-handed: right × forward = up ✓)
        glm::mat4 orient(1.0f);
        orient[0] = glm::vec4(forward, 0.0f);
        orient[1] = glm::vec4(up,      0.0f);
        orient[2] = glm::vec4(right,   0.0f);
        model = glm::translate(glm::mat4(1.0f), pos) * orient;
    } else {
        glm::vec3 pos(0.0f);
        if (!pos_key.empty()) {
            const auto* pv = context.TryGet<nlohmann::json>(pos_key);
            if (pv && pv->is_array() && pv->size() >= 3)
                pos = glm::vec3((*pv)[0].get<float>(), (*pv)[1].get<float>(), (*pv)[2].get<float>());
        }
        float yaw = 0.0f;
        if (!yaw_key.empty()) yaw = context.Get<float>(yaw_key, 0.0f);
        model = glm::translate(glm::mat4(1.0f), pos)
              * glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    auto* shadowTex  = context.Get<SDL_GPUTexture*>("shadow_depth_texture", nullptr);
    auto* shadowSamp = context.Get<SDL_GPUSampler*>("shadow_depth_sampler", nullptr);

    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    DrawMd3Surfaces(prefix, frame, model, view, proj, camPos, shadowVP, fu,
                    pass, cmd, shadowTex, shadowSamp, context);
}

}  // namespace sdl3cpp::services::impl
