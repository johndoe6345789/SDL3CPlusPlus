#include "services/interfaces/workflow/gta5/gta5_map_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_lead.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"
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
    if (tab && !held_) open_ = !open_;
    held_ = tab;
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
        LoadGta5MapOverlay(map_, device,
                           SDL_GetGPUSwapchainTextureFormat(device, window),
                           Gta5ParameterOr(step, "minimap_dir", ""),
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
    // Engine x is east and -z north: GTA's y, the map's up.
    const float minX = Gta5NumberOr(step, "map_min_x", -4140.f);
    const float maxY = Gta5NumberOr(step, "map_max_y", 8400.f);
    const float width = Gta5NumberOr(step, "map_width", 9000.f);
    const float height = Gta5NumberOr(step, "map_height", 13500.f);
    const std::vector<float> quads = BuildGta5MapQuads(
        static_cast<int>(context.Get<uint32_t>("frame_width", 1280u)),
        static_cast<int>(context.Get<uint32_t>("frame_height", 960u)),
        (at.x - minX) / width, (maxY + at.z) / height,
        std::atan2(ahead.x, -ahead.z));
    DrawGta5MapOverlay(map_, device, cmd, swapchain, quads);
}

}  // namespace sdl3cpp::services::impl
