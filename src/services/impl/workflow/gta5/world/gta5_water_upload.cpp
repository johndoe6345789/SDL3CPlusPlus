#include "services/interfaces/workflow/gta5/world/gta5_water_step.hpp"

#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/world/gta5_water.hpp"

namespace sdl3cpp::services::impl {

void WorkflowGta5WaterDrawStep::Load(const WorkflowStepDefinition& step,
                                     SDL_GPUDevice* device) {
    tried_ = true;
    const std::string path = Gta5ParameterOr(step, "water_file", "");
    const auto quads = LoadGta5WaterQuads(path);
    const auto mesh = BuildGta5WaterMesh(quads);
    if (logger_) {
        logger_->Info("gta5.water.draw: " + std::to_string(quads.size()) +
                      " water quads, " + std::to_string(mesh.size() / 3) +
                      " triangles, from '" + path + "'");
    }
    if (mesh.empty()) return;
    const auto bytes = static_cast<Uint32>(mesh.size() * sizeof(mesh[0]));
    SDL_GPUBufferCreateInfo info = {};
    info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    info.size = bytes;
    vertices_ = SDL_CreateGPUBuffer(device, &info);
    // Staged now, copied when gta5.tiles.cull next submits: in here, a
    // render pass is open and no copy can start.
    if (vertices_ &&
        state_->uploads.StageBuffer(device, mesh.data(), bytes, vertices_, 0)) {
        count_ = static_cast<std::uint32_t>(mesh.size());
    }
}

}  // namespace sdl3cpp::services::impl
