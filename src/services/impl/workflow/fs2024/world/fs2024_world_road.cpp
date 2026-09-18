#include "services/interfaces/workflow/fs2024/world/fs2024_world_gpu.hpp"

#include "services/interfaces/workflow/fs2024/data/texture/fs2024_dds_blocks.hpp"
#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_texture.hpp"

#include <exception>
#include <filesystem>

namespace sdl3cpp::services::impl {

void UploadFs2024RoadTexture(SDL_GPUDevice* device, Fs2024World& world) {
    const std::string path =
        world.paths.texSynthRoot + "/ROAD_ASPHALT00.JPG.DDS";
    if (!std::filesystem::exists(path)) return;
    try {
        const auto map = UploadFs2024LandmarkTexture(
            device, sdl3cpp::fs2024::ReadDdsBlocks(path));
        world.roadTexture = map.texture;
        world.roadSampler = map.sampler;
    } catch (const std::exception&) {
        // Roads then draw with the building kit's walls instead.
    }
}

}  // namespace sdl3cpp::services::impl
