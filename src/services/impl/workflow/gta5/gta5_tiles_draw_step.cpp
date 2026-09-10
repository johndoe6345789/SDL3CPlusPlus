#include "services/interfaces/workflow/gta5/gta5_tiles_draw_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_draw_instances.hpp"
#include "services/interfaces/workflow/gta5/gta5_draw_probe.hpp"
#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5TilesDrawStep::WorkflowGta5TilesDrawStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5TilesDrawStep::GetPluginId() const {
    return "gta5.tiles.draw";
}

void WorkflowGta5TilesDrawStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& context) {
    if (!state_ || context.GetBool("frame_skip", false)) return;

    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        Gta5ParameterOr(step, "pipeline_key", "gpu_pipeline_textured"),
        nullptr);

    Gta5DrawContext draw;
    draw.pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    draw.cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    if (!pipeline || !draw.pass || !draw.cmd) return;

    draw.view = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f));
    draw.proj = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.f));
    draw.shadowVP = context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.f));
    draw.cameraPos =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.f));
    draw.fragUniforms = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});
    draw.texture = context.Get<SDL_GPUTexture*>(
        Gta5ParameterOr(step, "texture_key", "walls_texture_gpu"), nullptr);
    draw.sampler = context.Get<SDL_GPUSampler*>(
        Gta5ParameterOr(step, "sampler_key", "walls_texture_sampler"), nullptr);
    draw.shadowTexture =
        context.Get<SDL_GPUTexture*>("shadow_depth_texture", nullptr);
    draw.shadowSampler =
        context.Get<SDL_GPUSampler*>("shadow_depth_sampler", nullptr);

    if (!draw.texture || !draw.sampler) {
        // Without both the pipeline's fragment samplers are unbound and
        // every draw would be discarded, so say so rather than drawing
        // nothing quietly.
        if (logger_ && state_->lastDrawLogged != -2) {
            state_->lastDrawLogged = -2;
            logger_->Warn("gta5.tiles.draw: no texture/sampler bound; "
                          "expected keys walls_texture_gpu and "
                          "walls_texture_sampler");
        }
        return;
    }

    SDL_BindGPUGraphicsPipeline(draw.pass, pipeline);
    const int drawn = DrawGta5Instances(*state_, draw);
    context.Set("gta5.tiles.drawn_last_frame", drawn);

    if (logger_ && drawn != state_->lastDrawLogged) {
        state_->lastDrawLogged = drawn;
        std::size_t instances = 0;
        for (const auto& entry : state_->resident) {
            instances += entry.second.instances.size();
        }
        logger_->Info("gta5.tiles.draw: drew " + std::to_string(drawn) +
                      " of " + std::to_string(instances) + " instances; " +
                      ProbeGta5FirstInstance(*state_, draw));
    }
}

}  // namespace sdl3cpp::services::impl
