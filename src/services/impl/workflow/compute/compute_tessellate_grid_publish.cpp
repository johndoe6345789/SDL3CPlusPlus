#include "services/interfaces/workflow/compute/compute_tessellate_grid.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

void PublishTessellationGrid(WorkflowContext& context,
                             const TessellationGridParams& params,
                             const TessellationGridBuffers& buffers) {
    context.Set<SDL_GPUBuffer*>("plane_" + params.name + "_vb",
                                buffers.vertexBuffer);
    context.Set<SDL_GPUBuffer*>("plane_" + params.name + "_ib",
                                buffers.indexBuffer);

    context.Set(
        "plane_" + params.name,
        nlohmann::json{{"vertex_count", buffers.vertexCount},
                       {"index_count", buffers.indexCount},
                       {"stride", buffers.vertexStride},
                       {"width", params.width},
                       {"depth", params.depth},
                       {"subdivisions", params.subdivisions},
                       {"displacement_strength", params.displacementStrength},
                       {"compute_tessellated", true}});
}

}  // namespace sdl3cpp::services::impl
