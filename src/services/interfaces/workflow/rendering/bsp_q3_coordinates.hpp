#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <array>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// Q3 is Z-up; the engine is Y-up. x'=x, y'=z, z'=-y, all scaled.
std::array<float, 3> ConvertQ3Point(float qx, float qy, float qz, float scale);

/// [x, y, z] as a JSON array.
nlohmann::json PointJson(const std::array<float, 3>& p);

/// Converts a brush model's Q3-space `mins`/`maxs` into an engine-space
/// axis-aligned bounding box by transforming all 8 corners and taking their
/// min/max, since the Z-up -> Y-up conversion isn't axis-preserving.
nlohmann::json ConvertModelBounds(const BspModel& model, float scale);

}  // namespace sdl3cpp::services::impl
