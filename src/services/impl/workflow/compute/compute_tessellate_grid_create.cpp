#include "services/interfaces/workflow/compute/compute_tessellate_grid_internal.hpp"

namespace sdl3cpp::services::impl {

TessellationGridBuffers CreateAndUploadTessellationGrid(SDL_GPUDevice* device,
                                                        int subdivisions) {
    namespace detail = tessellate_grid_detail;

    TessellationGridBuffers buffers =
        detail::AllocateGridBuffers(device, subdivisions);
    detail::UploadGridIndices(device, subdivisions, buffers);
    return buffers;
}

}  // namespace sdl3cpp::services::impl
