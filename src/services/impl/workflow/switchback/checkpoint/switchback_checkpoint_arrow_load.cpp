#include "services/interfaces/workflow/switchback/checkpoint/switchback_checkpoint_arrow_step.hpp"

#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow/switchback/checkpoint/switchback_arrow_mesh.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

void WorkflowSwitchbackCheckpointArrowStep::LoadOnce(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) return;
    loaded_ = true;
    radius_ = Gta5NumberOr(step, "radius", 14.f);
    const std::string path = Gta5ResolvePath(
        step, context, "map",
        "packages/switchback/assets/spiral_pass_map.json");
    if (!route_.Load(path)) {
        if (logger_) {
            logger_->Error("switchback.checkpoint.arrow: no checkpoints in " +
                           path);
        }
        return;
    }
    LoadMarquees(device);
    const GeometryPlaneMesh mesh = BuildSwitchbackArrowMesh();
    try {
        arrow_ = UploadGeometryPlaneMesh(device, mesh);
    } catch (const std::runtime_error& error) {
        if (logger_) {
            logger_->Error(std::string("switchback.checkpoint.arrow: ") +
                           error.what());
        }
        return;
    }
    arrowIndexCount_ = static_cast<std::uint32_t>(mesh.indices.size());
    if (logger_) {
        logger_->Trace("WorkflowSwitchbackCheckpointArrowStep", "LoadOnce",
                       "radius=" + std::to_string(radius_),
                       "Checkpoint arrow ready");
    }
}

}  // namespace sdl3cpp::services::impl
