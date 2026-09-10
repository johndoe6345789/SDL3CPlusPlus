#pragma once

/// Internal helpers shared by the compute_tessellate_grid_*.cpp files that
/// together implement CreateAndUploadTessellationGrid() from
/// compute_tessellate_grid.hpp. Not part of the public workflow-step API.

#include "services/interfaces/workflow/compute/compute_tessellate_grid.hpp"

namespace sdl3cpp::services::impl::tessellate_grid_detail {

/// Allocates the vertex and index buffers (sized from `subdivisions`) and
/// fills in vertexCount/indexCount/vertexStride; leaves both buffers
/// uninitialized (the index buffer is filled by UploadGridIndices()).
/// @throws std::runtime_error if either buffer fails to create.
TessellationGridBuffers AllocateGridBuffers(SDL_GPUDevice* device,
                                            int subdivisions);

/// Generates the CPU-side triangle-grid index data and uploads it into
/// `buffers.indexBuffer` via a one-shot transfer buffer.
/// @throws std::runtime_error if the transfer buffer fails to create.
void UploadGridIndices(SDL_GPUDevice* device, int subdivisions,
                       const TessellationGridBuffers& buffers);

}  // namespace sdl3cpp::services::impl::tessellate_grid_detail
