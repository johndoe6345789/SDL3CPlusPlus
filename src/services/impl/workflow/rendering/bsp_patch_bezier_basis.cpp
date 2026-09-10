#include "services/interfaces/workflow/rendering/bsp_patch_bezier_basis.hpp"

namespace sdl3cpp::services::impl {

BezierWeights BasisWeights(float t) {
    return {(1.0f - t) * (1.0f - t), 2.0f * (1.0f - t) * t, t * t};
}

BezierWeights DerivativeWeights(float t) {
    return {-2.0f * (1.0f - t), 2.0f - 4.0f * t, 2.0f * t};
}

}  // namespace sdl3cpp::services::impl
