#pragma once

#include "services/interfaces/workflow/gta5/gta5_resource.hpp"

#include <array>
#include <cstdint>

namespace sdl3cpp::services::impl {

/// The name hash of the texture a G9 shader samples as its diffuse, or 0.
///
/// A shader's parameters are described at +0x20: an 8-byte header whose
/// second byte counts textures and fifth counts parameters, then a (name
/// hash, data) pair each. A texture parameter has data & 3 == 0, and
/// (data >> 2) & 0xFF indexes the texture pointers at +0x10, each a
/// texture with its name at +0x28. Gen9 renamed the samplers:
/// DiffuseSampler is DiffuseTex, terrain's TextureSampler_layer0 is
/// DiffuseTexture_layer0. With neither, 0: guessing drew normal maps.
std::uint32_t ReadGta5DiffuseTexture(const Gta5Resource& res,
                                     std::int64_t shader);

/// A terrain shader's four diffuse layers (DiffuseTexture_layer0..3), then
/// its lookup mask (lookupTexture), 0 for any it lacks. True when it has
/// at least two layers: a terrain shader.
bool ReadGta5TerrainLayers(const Gta5Resource& res, std::int64_t shader,
                           std::array<std::uint32_t, 5>& layers);

}  // namespace sdl3cpp::services::impl
