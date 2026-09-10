#include "services/interfaces/workflow/rendering/gpu_overlay_quad.hpp"

namespace sdl3cpp::services::impl {

GpuOverlayQuad BuildGpuOverlayQuad(int viewportWidth, int viewportHeight,
                                   int overlayWidth, int overlayHeight,
                                   float margin) {
    const float vw = static_cast<float>(viewportWidth > 0 ? viewportWidth : 1);
    const float vh =
        static_cast<float>(viewportHeight > 0 ? viewportHeight : 1);

    const float pxLeft   = vw - static_cast<float>(overlayWidth) - margin;
    const float pxRight  = vw - margin;
    const float pxTop    = margin;
    const float pxBottom = margin + static_cast<float>(overlayHeight);

    const float x0 = pxLeft / vw * 2.0f - 1.0f;
    const float x1 = pxRight / vw * 2.0f - 1.0f;
    const float y1 = 1.0f - pxTop / vh * 2.0f;
    const float y0 = 1.0f - pxBottom / vh * 2.0f;

    return GpuOverlayQuad{
        x0, y1, 0.0f, 0.0f, 0.0f,  // top-left
        x1, y1, 0.0f, 1.0f, 0.0f,  // top-right
        x1, y0, 0.0f, 1.0f, 1.0f,  // bottom-right
        x0, y1, 0.0f, 0.0f, 0.0f,  // top-left
        x1, y0, 0.0f, 1.0f, 1.0f,  // bottom-right
        x0, y0, 0.0f, 0.0f, 1.0f,  // bottom-left
    };
}

}  // namespace sdl3cpp::services::impl
