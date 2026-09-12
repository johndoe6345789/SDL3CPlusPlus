#pragma once

#include "services/interfaces/workflow/gta5/resource/gta5_placement.hpp"

#include <array>

namespace sdl3cpp::services::impl {

/// Translate * Rotate * Scale, flattened the way SceneObject stores it.
std::array<float, 16> BuildGta5ModelMatrix(const Gta5Placement& placement);

}  // namespace sdl3cpp::services::impl
