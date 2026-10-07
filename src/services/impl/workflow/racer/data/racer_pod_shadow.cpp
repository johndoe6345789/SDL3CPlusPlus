#include "racer_model_context.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl::racer_model_detail {

void NotePodShadow(ModelWalk& walk, std::uint32_t mesh) {
    // A pod's root selector holds, besides its parts, flat quads the
    // game draws as shadows: the whole pod's and the cockpit's alone.
    const RacerBigEndianReader& r = walk.reader;
    if (!walk.IsPointer(mesh, 0x40)) return;
    const std::uint32_t vertices = r.U32(mesh + 0x34);
    if (r.I16(mesh + 0x3A) != 4 || !walk.IsPointer(vertices, 4 * 16)) {
        return;
    }
    float lo[3] = {1e9f, 1e9f, 1e9f};
    float hi[3] = {-1e9f, -1e9f, -1e9f};
    for (std::uint32_t v = 0; v < 4; ++v) {
        for (std::uint32_t k = 0; k < 3; ++k) {
            const float c = r.I16(vertices + 16 * v + 2 * k);
            lo[k] = std::min(lo[k], c);
            hi[k] = std::max(hi[k], c);
        }
    }
    const bool flat = hi[2] == lo[2];
    if (flat && hi[1] - lo[1] > walk.shadowLength) {
        walk.shadowWidth = hi[0] - lo[0];
        walk.shadowLength = hi[1] - lo[1];
    }
}

}  // namespace sdl3cpp::services::impl::racer_model_detail
