#include "services/interfaces/workflow/rendering/workflow_render_grid_draw_step.hpp"
#include "services/interfaces/workflow/rendering/grid_draw_render.hpp"

#include <nlohmann/json.hpp>
#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowRenderGridDrawStep::WorkflowRenderGridDrawStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowRenderGridDrawStep::GetPluginId() const {
    return "render.grid.draw";
}

void WorkflowRenderGridDrawStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    // Read grid config (populated by render.grid.setup)
    const auto* configPtr = context.TryGet<nlohmann::json>("grid.config");
    if (!configPtr || !configPtr->is_object()) {
        throw std::runtime_error(
            "render.grid.draw: grid.config not found "
            "(run render.grid.setup first)");
    }
    const GridDrawConfig cfg = ReadGridDrawConfig(*configPtr);

    glm::mat4 view, proj;
    ReadGridCameraMatrices(context, view, proj);

    const GridGpuResources gpu = ReadGridGpuResources(context);
    if (!gpu.IsComplete()) {
        throw std::runtime_error(
            "render.grid.draw: Missing GPU resources "
            "(run render.grid.setup first)");
    }

    const float time =
        static_cast<float>(context.GetDouble("frame.elapsed", 0.0));

    const uint32_t drawCalls = DrawGridCubes(gpu, cfg, view, proj, time);

    // Store per-frame stats
    context.Set<uint32_t>("grid.draw_calls", drawCalls);
    context.Set<uint32_t>("grid.cubes_drawn", cfg.gridWidth * cfg.gridHeight);

    // Frame counter management for loop termination
    uint32_t frameNum = context.Get<uint32_t>("frame.number", 0u);
    frameNum++;
    context.Set<uint32_t>("frame.number", frameNum);

    // Advance elapsed time (fixed timestep ~60fps)
    double elapsed = context.GetDouble("frame.elapsed", 0.0);
    context.Set<double>("frame.elapsed", elapsed + (1.0 / 60.0));

    // Check loop termination against num_frames from grid config
    if (frameNum >= cfg.numFrames) {
        context.Set<bool>("grid.running", false);
    }

    if (logger_) {
        if (frameNum % 100 == 0) {
            logger_->Trace("WorkflowRenderGridDrawStep", "Execute",
                           "frame=" + std::to_string(frameNum) +
                               ", draw_calls=" + std::to_string(drawCalls));
        }
    }
}

}  // namespace sdl3cpp::services::impl
