#pragma once

#include <assimp/material.h>

#include <array>

namespace sdl3cpp::services::impl {

/// A material's tint and alpha-discard threshold, as one vec4.
///
/// rgb multiplies the texture and a is the threshold, 0 meaning draw
/// everything. They travel together because the draw pushes them as a
/// single uniform.
std::array<float, 4> ReadGta5MaterialSurface(const aiMaterial& material);

}  // namespace sdl3cpp::services::impl
