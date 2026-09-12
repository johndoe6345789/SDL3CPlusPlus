#include "services/interfaces/workflow/gta5/world/gta5_water_map_step.hpp"

#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5WaterMapStep::WorkflowGta5WaterMapStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5WaterMapStep::GetPluginId() const {
    return "gta5.water.map";
}

void WorkflowGta5WaterMapStep::Build(const WorkflowStepDefinition& step,
                                     SDL_GPUDevice* device) {
    tried_ = true;
    water_ = LoadGta5WaterQuads(Gta5ParameterOr(step, "water_file", ""));
    const Gta5WaterGrid grid;
    std::vector<float> heights;
    RasterGta5WaterHeights(water_, grid, heights);
    SDL_GPUTextureCreateInfo info = {};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = SDL_GPU_TEXTUREFORMAT_R32_FLOAT;
    info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    info.width = static_cast<Uint32>(grid.columns);
    info.height = static_cast<Uint32>(grid.rows);
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    map_ = SDL_CreateGPUTexture(device, &info);
    SDL_GPUSamplerCreateInfo nearest = {};  // heights, not colours: no blend
    nearest.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    nearest.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_ = SDL_CreateGPUSampler(device, &nearest);
    const auto bytes = static_cast<std::uint32_t>(heights.size() * 4);
    if (!map_ || !sampler_ ||
        !state_->uploads.StageTexture(device, heights.data(), bytes, map_, 0,
                                      info.width, info.height)) {
        if (logger_) logger_->Warn("gta5.water.map: no water map");
        return;
    }
    if (logger_) {
        logger_->Info("gta5.water.map: " + std::to_string(water_.size()) +
                      " water quads into a " + std::to_string(grid.columns) +
                      "x" + std::to_string(grid.rows) + " height grid");
    }
}

void WorkflowGta5WaterMapStep::Execute(const WorkflowStepDefinition& step,
                                       WorkflowContext& context) {
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!state_ || !device) return;
    if (!tried_) Build(step, device);
    if (!map_ || !sampler_) return;
    context.Set<SDL_GPUTexture*>("gta5.water.map", map_);
    context.Set<SDL_GPUSampler*>("gta5.water.map_sampler", sampler_);
    // How deep the camera is: GTA's y is engine -z.
    const auto eye =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.f));
    float surface = 0.f;
    const bool wet = Gta5WaterHeightAt(water_, eye.x, -eye.z, surface);
    context.Set<float>("gta5.camera.underwater",
                       wet ? surface - eye.y : -1.f);
}

}  // namespace sdl3cpp::services::impl
