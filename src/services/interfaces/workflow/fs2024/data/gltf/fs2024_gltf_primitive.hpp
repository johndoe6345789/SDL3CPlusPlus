#pragma once

#include "services/interfaces/workflow/fs2024/data/gltf/fs2024_gltf_model.hpp"

#include <glm/mat4x4.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::fs2024 {

/// One mesh primitive in this engine's vertex format, positions and
/// normals already through `transform` (its node's world transform).
GltfPrimitive BuildGltfPrimitive(const nlohmann::json& gltf,
                                 const std::uint8_t* bin,
                                 const nlohmann::json& prim,
                                 const glm::mat4& transform);

/// The primitive's own triangles. FS2024 packs every primitive of a
/// mesh that shares vertex data into ONE "indices" accessor and tells
/// them apart with its own `ASOBO_primitive` extra: `StartIndex` and
/// `PrimitiveCount` (index elements / triangles) name each one's slice.
/// Taking the whole accessor, as plain glTF would, draws every
/// primitive with all of the mesh's geometry.
std::vector<std::uint32_t> ReadGltfPrimitiveIndices(
    const nlohmann::json& gltf, const std::uint8_t* bin,
    const nlohmann::json& prim);

/// The image URI of the primitive's base-colour texture, or empty.
/// FS2024's DDS textures are not a core glTF image type, so
/// `MSFT_texture_dds` carries the real image index, not `source`.
std::string GltfBaseColorImageUri(const nlohmann::json& gltf,
                                  const nlohmann::json& prim);

}  // namespace sdl3cpp::fs2024
