#include "services/interfaces/workflow/quake3/q3_md3_surface_uploader.hpp"

#include "services/interfaces/workflow/quake3/q3_md3_surface_geometry.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_surface_texture.hpp"

namespace sdl3cpp::services::impl {

void UploadMd3Surface(SDL_GPUDevice* device, const Q3Md3Source& source,
                      const uint8_t* surfacePtr, const q3::Md3Surface& surface,
                      int frameCount, const std::string& keyPrefix,
                      WorkflowContext& context) {
    if (surface.numVerts <= 0 || surface.numTriangles <= 0) {
        return;
    }
    UploadIndexBuffer(device, surfacePtr, surface, keyPrefix, context);
    UploadVertexBuffers(device, surfacePtr, surface, frameCount, keyPrefix,
                        context);
    UploadSurfaceTexture(device, source, surfacePtr, surface, keyPrefix,
                         context);
}

}  // namespace sdl3cpp::services::impl
