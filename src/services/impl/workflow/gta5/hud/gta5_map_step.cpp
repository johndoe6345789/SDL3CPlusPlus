#include "services/interfaces/workflow/gta5/hud/gta5_map_step.hpp"

#include "services/interfaces/workflow/gta5/hud/gta5_map_build.hpp"
#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_lead.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_composite_state.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>

#include <cmath>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5MapDrawStep::WorkflowGta5MapDrawStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5MapDrawStep::GetPluginId() const {
    return "gta5.map.draw";
}

void WorkflowGta5MapDrawStep::Execute(const WorkflowStepDefinition& step,
                                      WorkflowContext& context) {
    if (!state_) return;
    const bool tab = Gta5KeyDown(
        context.TryGet<nlohmann::json>("input.keyboard.state"), "Tab");
    if (tab && !held_) {
        open_ = !open_;
        if (logger_) logger_->Info(open_ ? "gta5.map: open" : "gta5.map: shut");
    }
    held_ = tab;
    const Gta5MapRect rect = Gta5MapRectFor(step);
    // A double-click on the map travels there, and closes it.
    if (TravelGta5Map(context, travel_, open_, rect)) {
        open_ = false;
        if (logger_) logger_->Info("gta5.map: shut, travelling");
    }
    if (!open_ || context.GetBool("frame_skip", false) ||
        context.GetString(kPostfxCompositeStateKey) !=
            kPostfxCompositeStateDrawn) {
        return;
    }
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* swapchain =
        context.Get<SDL_GPUTexture*>("postfx_swapchain_texture", nullptr);
    if (!device || !window || !cmd || !swapchain) return;
    if (!map_.tried) {
        LoadGta5MapOverlay(
            map_, device, SDL_GetGPUSwapchainTextureFormat(device, window),
            Gta5ParameterOr(step, "minimap_dir", ""),
            Gta5ResolvePath(step, context, "poi_file",
                            "packages/gta5/config/map_poi.json"),
            state_->uploads, logger_);
    }
    if (!map_.ready) return;

    // Where: the car when seated, else the player. Which way: the camera.
    const auto* ps = context.TryGet<Q3PlayerState>("q3.ps");
    const glm::vec3 at =
        Gta5StreamOrigin(*state_, ps ? ps->origin : state_->centreOrigin);
    const auto view =
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f));
    const glm::vec3 ahead(-view[0][2], -view[1][2], -view[2][2]);
    const Gta5MapFrame frame = BuildGta5MapFrame(
        map_, rect,
        static_cast<int>(context.Get<uint32_t>("frame_width", 1280u)),
        static_cast<int>(context.Get<uint32_t>("frame_height", 960u)),
        Gta5MapCars(*state_), glm::vec2(at.x, -at.z),
        std::atan2(ahead.x, -ahead.z));
    DrawGta5MapOverlay(map_, device, cmd, swapchain, frame);
}

}  // namespace sdl3cpp::services::impl
