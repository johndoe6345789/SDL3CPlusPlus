#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// geometry.create_plane's resolved parameters, each with the original
/// defaults.
struct GeometryPlaneParams {
    float width      = 10.0f;
    float depth      = 10.0f;
    float uvScaleX   = 1.0f;
    float uvScaleY   = 1.0f;
    int subdivisionsX = 1;
    int subdivisionsY = 1;
    std::string name  = "plane";
};

GeometryPlaneParams ReadGeometryPlaneParams(const WorkflowStepDefinition& step);

/// Vertex format shared with the other plane/mesh creation steps: float3
/// position + float2 uv = 20 bytes.
struct PlanePosUvVertex {
    float x, y, z;
    float u, v;
};

/// A subdivided XZ-plane mesh, centered on the origin, with `y` = 0.
struct GeometryPlaneMesh {
    std::vector<PlanePosUvVertex> vertices;
    std::vector<uint16_t> indices;
};

/// Builds the grid of `(subdivisionsX+1) * (subdivisionsY+1)` vertices and
/// their two-triangle-per-cell indices, exactly as geometry.create_plane
/// always has.
GeometryPlaneMesh BuildGeometryPlaneMesh(const GeometryPlaneParams& params);

/// The GPU vertex/index buffers created for one uploaded plane mesh.
struct GeometryPlaneBuffers {
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer  = nullptr;
};

/**
 * @brief Creates and uploads `mesh` via one transfer buffer and copy pass.
 *
 * Throws std::runtime_error (prefixed "geometry.create_plane: ...") if any
 * GPU resource fails to create, releasing whatever was already created
 * first, exactly as the original inline code did.
 */
GeometryPlaneBuffers UploadGeometryPlaneMesh(SDL_GPUDevice* device,
                                             const GeometryPlaneMesh& mesh);

}  // namespace sdl3cpp::services::impl
