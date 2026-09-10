#pragma once

/// Internal helper shared by geometry_plane_buffer_create.cpp and
/// geometry_plane_upload.cpp, which together implement
/// UploadGeometryPlaneMesh() from geometry_plane_helpers.hpp. Not part of
/// the public workflow-step API.

#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"

namespace sdl3cpp::services::impl::geometry_plane_detail {

/// Allocates the vertex, index and transfer buffers for `mesh`.
/// @throws std::runtime_error if any of the three fails to create.
GeometryPlaneBuffers CreateMeshBuffers(SDL_GPUDevice* device,
                                       const GeometryPlaneMesh& mesh,
                                       SDL_GPUTransferBuffer*& outTransfer);

}  // namespace sdl3cpp::services::impl::geometry_plane_detail
