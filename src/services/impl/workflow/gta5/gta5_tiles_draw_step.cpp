#include "services/interfaces/workflow/gta5/gta5_tiles_draw_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_draw_context_build.hpp"
#include "services/interfaces/workflow/gta5/gta5_draw_instances.hpp"
#include "services/interfaces/workflow/gta5/gta5_draw_probe.hpp"
#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"

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
    const Gta5DrawContext draw = BuildGta5DrawContext(step, context);
    if (!pipeline || !draw.pass || !draw.cmd) return;

    if (!draw.texture || !draw.sampler) {
        // Without both, the pipeline's fragment samplers are unbound and
        // every draw is discarded, so say so rather than drawing nothing
        // quietly.
        if (logger_ && state_->lastDrawLogged != -2) {
            state_->lastDrawLogged = -2;
            logger_->Warn("gta5.tiles.draw: no texture/sampler bound; "
                          "expected walls_texture_gpu and "
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
                      " of " + std::to_string(instances) + " instances, y=" +
                      std::to_string(state_->centreOrigin.y));
    }
}

}  // namespace sdl3cpp::services::impl
