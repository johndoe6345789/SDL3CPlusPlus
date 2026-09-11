#pragma once

#include "services/interfaces/workflow/gta5/gta5_map_overlay.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// An RGBA8 texture of `pixels`, rows top first, staged in `uploads`.
SDL_GPUTexture* CreateGta5MapRgba(SDL_GPUDevice* device, int width,
                                  int height,
                                  const std::vector<std::uint8_t>& pixels,
                                  Gta5UploadBatch& uploads);

/// The player's arrow -- yellow, pointing up -- and a one-texel backdrop.
bool CreateGta5MapMarkers(Gta5MapOverlay& map, SDL_GPUDevice* device,
                          Gta5UploadBatch& uploads);

/// One atlas: a round lettered icon per category in 32-pixel cells along
/// the top, then the lettering's glyphs in 6 x 8 cells below.
bool CreateGta5MapAtlas(Gta5MapOverlay& map, SDL_GPUDevice* device,
                        Gta5UploadBatch& uploads);

/// A category's icon in the atlas: u0, v0, u1, v1.
glm::vec4 Gta5MapIconUv(int category);

/// A glyph's box in the atlas; false for a blank, which draws nothing.
bool Gta5MapGlyphUv(char c, glm::vec4& uv);

}  // namespace sdl3cpp::services::impl
