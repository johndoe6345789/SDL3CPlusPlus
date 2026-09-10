#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Name of the sky shader used by a BSP, or empty if it has none.
 *
 * Quake keeps sky shaders under textures/skies/, and a map has at most
 * one, so the first match in the texture lump is it. The name is a
 * shader name, not a file: the image behind it comes from the shader
 * script's first stage.
 */
std::string FindSkyTextureName(
    const std::shared_ptr<std::vector<uint8_t>>& bspData);

}  // namespace sdl3cpp::services::impl
