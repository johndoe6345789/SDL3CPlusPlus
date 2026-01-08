#ifndef SDL3CPP_CORE_VERTEX_HPP
#define SDL3CPP_CORE_VERTEX_HPP

#include <array>

namespace sdl3cpp::core {

struct Vertex {
    std::array<float, 3> position;
    std::array<float, 3> normal;
    std::array<float, 3> tangent;
    std::array<float, 2> texcoord;
    std::array<float, 3> color;
};

struct PushConstants {
    std::array<float, 16> model;
    std::array<float, 16> viewProj;
    // Extended fields for PBR/atmospherics (optional for basic shaders)
    std::array<float, 16> view;
    std::array<float, 16> proj;
    std::array<float, 16> lightViewProj;
    std::array<float, 3> cameraPos;
    float time;
    // Atmospherics parameters
    float ambientStrength;
    float fogDensity;
    float fogStart;
    float fogEnd;
    std::array<float, 3> fogColor;
    float gamma;
    float exposure;
    int enableShadows;
    int enableFog;
};

// static_assert(sizeof(PushConstants) == sizeof(float) * 95, "push constant size mismatch");

} // namespace sdl3cpp::core

#endif // SDL3CPP_CORE_VERTEX_HPP
