#pragma once

#include "services/interfaces/workflow/gta5/gta5_resource.hpp"

#include <cstdint>

namespace sdl3cpp::services::impl {

/// The name hash of the texture a G9 shader samples as its diffuse, or 0.
///
/// A shader's parameters are described at +0x20: an 8-byte header whose
/// fifth byte counts them, then a (name hash, data) pair each. A texture
/// parameter has data & 3 == 0, and (data >> 2) & 0xFF indexes the
/// texture pointers at +0x10, each a texture with its name at +0x28. The
/// one bound to DiffuseSampler -- TextureSampler_layer0 on terrain -- is
/// the diffuse. Taking the first texture that was not a normal or
/// specular map instead picked terrain blend masks and tint palettes.
/// Falls back to that when no parameter is named either.
std::uint32_t ReadGta5DiffuseTexture(const Gta5Resource& res,
                                     std::int64_t shader);

}  // namespace sdl3cpp::services::impl
