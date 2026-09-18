#pragma once

#include "services/interfaces/workflow/fs2024/data/gltf/fs2024_gltf_model.hpp"

#include <cstddef>
#include <cstdint>

namespace sdl3cpp::fs2024 {

/// Decodes one LOD's binary glTF (`glTF` header, JSON chunk, BIN chunk)
/// into primitives, every node's world transform applied.
GltfLod ParseLodGlb(const std::uint8_t* data, std::size_t size);

}  // namespace sdl3cpp::fs2024
