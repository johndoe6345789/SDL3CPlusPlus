#include "services/interfaces/workflow/quake3/q3_overlay_scale.hpp"

#include <algorithm>

namespace sdl3cpp::q3 {

OverlaySize ChooseOverlaySize(int targetWidth, int targetHeight) {
    const int byWidth = targetWidth / kVirtualWidth;
    const int byHeight = targetHeight / kVirtualHeight;

    // The smaller axis decides, so the overlay never rasterises larger
    // than the surface it will be presented on.
    int scale = std::min(byWidth, byHeight);
    scale = std::clamp(scale, 1, kMaxOverlayScale);

    return OverlaySize{kVirtualWidth * scale, kVirtualHeight * scale, scale};
}

}  // namespace sdl3cpp::q3
