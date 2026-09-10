#include "services/interfaces/workflow/quake3/workflow_q3_md3_draw_step.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_anim_frame.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_draw_params.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_draw_surfaces.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_model_matrix.hpp"
#include "services/interfaces/workflow/rendering/pk3_bsp_loader.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowQ3Md3DrawStep::WorkflowQ3Md3DrawStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3Md3DrawStep::GetPluginId() const {
    return "q3.md3.draw";
}

void WorkflowQ3Md3DrawStep::Execute(const WorkflowStepDefinition& step,
                                    WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    Md3DrawParams params = ReadMd3DrawParams(step);
    // The viewmodel's prefix follows the selected weapon, so allow it to
    // be bound to a context key as well as fixed in the step.
    params.prefix =
        GetStringParamOrInput(step, context, "prefix", params.prefix);
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline =
        context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_textured", nullptr);
    if (!pass || !cmd || !pipeline) return;
    const int nFrames =
        context.Get<int>("q3.md3." + params.prefix + "_num_frames", 0);
    if (nFrames <= 0) return;

    const int frame = ResolveMd3AnimFrame(context, params, nFrames);
    auto view   = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
    auto proj   = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    auto camPos = context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.0f));
    auto shadowVP = context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.0f));
    auto fu       = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});
    const glm::mat4 model = BuildMd3ModelMatrix(context, params, view, camPos);

    auto* shadowTex =
        context.Get<SDL_GPUTexture*>("shadow_depth_texture", nullptr);
    auto* shadowSamp =
        context.Get<SDL_GPUSampler*>("shadow_depth_sampler", nullptr);

    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    // The view weapon sits centimetres from the eye, so world geometry
    // wins the depth test and the gun buries itself in any wall the
    // player touches. Quake compresses it into the front of the depth
    // buffer instead (ioq3 tr_backend.c: glDepthRange(0, 0.3)); the same
    // trick here is the viewport's depth range.
    const auto viewW = context.Get<uint32_t>("frame_width", 1280u);
    const auto viewH = context.Get<uint32_t>("frame_height", 960u);
    if (params.viewmodel) {
        SDL_GPUViewport vp{
            0.0f, 0.0f, static_cast<float>(viewW), static_cast<float>(viewH),
            0.0f, 0.3f};
        SDL_SetGPUViewport(pass, &vp);
    }

    DrawMd3Surfaces(params.prefix, frame, model, view, proj, camPos, shadowVP,
                    fu, pass, cmd, shadowTex, shadowSamp, context);

    if (params.viewmodel) {
        SDL_GPUViewport full{
            0.0f, 0.0f, static_cast<float>(viewW), static_cast<float>(viewH),
            0.0f, 1.0f};
        SDL_SetGPUViewport(pass, &full);
    }
}

}  // namespace sdl3cpp::services::impl
