#pragma once

#include <array>

namespace sdl3cpp::services::impl {

struct MaterialXSurfaceParameters {
    std::array<float, 3> albedo = {1.0f, 1.0f, 1.0f};
    float roughness = 0.3f;
    float metallic = 0.0f;
    bool hasAlbedo = false;
    bool hasRoughness = false;
    bool hasMetallic = false;
};

}  // namespace sdl3cpp::services::impl
