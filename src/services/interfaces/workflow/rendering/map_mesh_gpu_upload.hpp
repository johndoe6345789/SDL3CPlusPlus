#pragma once

#include "services/interfaces/workflow/rendering/map_mesh_geometry.hpp"

#include <SDL3/SDL_gpu.h>

#include <vector>

namespace sdl3cpp::services::impl {

/// The GPU vertex/index buffers created for one uploaded map mesh.
struct MapMeshBuffers {
    SDL_GPUBuffer* vb = nullptr;
    SDL_GPUBuffer* ib = nullptr;
};

/// Creates and uploads `vertices`/`indices` via one transfer buffer and one
/// copy pass, exactly as map.load always has.
MapMeshBuffers UploadMapMeshBuffers(SDL_GPUDevice* device,
                                    const std::vector<PosUvVertex>& vertices,
                                    const std::vector<uint16_t>& indices);

}  // namespace sdl3cpp::services::impl
