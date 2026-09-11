#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <glm/glm.hpp>

#include <memory>

namespace sdl3cpp::services::impl {

/// F3: what the middle of the screen is looking at. Logs the nearest of
/// this frame's drawn instances whose bounding sphere the view ray
/// crosses -- archetype, distance, LOD range and each material's texture
/// size -- so a blurry surface can be named and traced to its data.
void LogGta5Pick(const Gta5StreamState& state, const glm::vec3& eye,
                 const glm::vec3& ahead,
                 const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
