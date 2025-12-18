#ifndef SDL3CPP_CORE_VERTEX_HPP
#define SDL3CPP_CORE_VERTEX_HPP

#include <array>

namespace sdl3cpp::core {

struct Vertex {
    std::array<float, 3> position;
    std::array<float, 3> color;
};

struct PushConstants {
    std::array<float, 16> model;
    std::array<float, 16> viewProj;
};

static_assert(sizeof(PushConstants) == sizeof(float) * 32, "push constant size mismatch");

} // namespace sdl3cpp::core

#endif // SDL3CPP_CORE_VERTEX_HPP
