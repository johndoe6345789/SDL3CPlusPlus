#include "services/interfaces/workflow/gta5/gta5_tiles_cull_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_timer.h>

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5TilesCullStep::WorkflowGta5TilesCullStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5TilesCullStep::GetPluginId() const {
    return "gta5.tiles.cull";
}

void WorkflowGta5TilesCullStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& context) {
    if (!state_) return;
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) return;
    // Everything staged this frame -- meshes, textures -- in one submit,
    // ahead of the frame that draws it.
    state_->uploads.Flush(device);
    if (context.GetBool("frame_skip", false)) return;
    const Gta5CostTimer timed(state_->cost.cull);

    const auto view =
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f));
    const auto proj =
        context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.f));
    const auto camera =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.f));
    // F2 toggles the collision view: what is solid, not what LOD shows.
    const bool toggle = Gta5KeyDown(
        context.TryGet<nlohmann::json>("input.keyboard.state"), "F2");
    if (toggle && !toggleHeld_) {
        collisionView_ = !collisionView_;
        if (logger_) {
            logger_->Info(collisionView_ ? "gta5.tiles.cull: collision view"
                                         : "gta5.tiles.cull: normal view");
        }
    }
    toggleHeld_ = toggle;
    Gta5CullOptions options;
    options.sizeRatio = Gta5NumberOr(step, "cull_size_ratio", 0.003f);
    options.lodScale = Gta5NumberOr(step, "lod_scale", 1.f);
    options.collision = collisionView_;
    BuildGta5InstanceBatch(*state_, proj * view, camera, options,
                           state_->batch);
    if (!UploadGta5InstanceBatch(device, state_->batch)) {
        // Nothing is drawn from a batch that did not upload.
        state_->batch.groups.clear();
        if (logger_ && !warned_) {
            warned_ = true;
            logger_->Warn(std::string("gta5.tiles.cull: instance upload "
                                      "failed: ") + SDL_GetError());
        }
    }
}

}  // namespace sdl3cpp::services::impl
