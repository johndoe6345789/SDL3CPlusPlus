#pragma once

#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Whether a BSP face with this texture name should be rendered.
 *
 * Q3 uses texture-name conventions rather than a dedicated flag for several
 * non-rendering surface kinds (sky, triggers, clip brushes, hint/areaportal
 * planes used only for visibility computation, and no-draw caulk).
 */
bool IsBspFaceTextureVisible(const std::string& textureName);

}  // namespace sdl3cpp::services::impl
