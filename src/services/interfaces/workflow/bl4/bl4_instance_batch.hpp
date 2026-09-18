#pragma once

#include "services/interfaces/workflow/bl4/bl4_geometry.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// One instanced draw: every visible copy of one submesh, as matrices
/// [first, first + count) of the batch's storage buffer.
struct Bl4DrawItem {
    const Bl4SubMesh* sub = nullptr;
    std::uint32_t first = 0;
    std::uint32_t count = 0;
};

/// This frame's visible instances, grouped by archetype so each submesh
/// is one draw call instead of one per placement -- the whole map is
/// tens of thousands of placements, and issuing a call each left the GPU
/// idle at 14% while the CPU fell behind.
struct Bl4InstanceBatch {
    std::vector<const Bl4Instance*> visible;
    std::vector<glm::mat4> matrices;
    std::vector<Bl4DrawItem> items;
    SDL_GPUBuffer* buffer = nullptr;
    SDL_GPUTransferBuffer* transfer = nullptr;
    std::uint32_t capacity = 0;  // in matrices
};

/// Groups `batch.visible` by geometry into matrices + draw items, then
/// orders the items by texture so each is bound once.
void BuildBl4InstanceBatch(Bl4InstanceBatch& batch);

/// Uploads the matrices on a command buffer of its own, submitted before
/// the frame's, growing the buffers as needed.
bool UploadBl4InstanceBatch(SDL_GPUDevice* device, Bl4InstanceBatch& batch);

/// Releases the GPU buffers (bl4.tiles.free, at shutdown).
void ReleaseBl4InstanceBatch(SDL_GPUDevice* device, Bl4InstanceBatch& batch);

}  // namespace sdl3cpp::services::impl
