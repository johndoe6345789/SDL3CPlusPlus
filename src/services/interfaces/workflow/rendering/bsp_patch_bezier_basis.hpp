#pragma once

namespace sdl3cpp::services::impl {

/// Quadratic Bezier basis and derivative weights for a single parameter.
struct BezierWeights {
    float w0, w1, w2;
};

/// Quadratic Bezier basis weights at parameter `t`.
BezierWeights BasisWeights(float t);

/// Derivative of the quadratic Bezier basis at parameter `t`.
BezierWeights DerivativeWeights(float t);

}  // namespace sdl3cpp::services::impl
