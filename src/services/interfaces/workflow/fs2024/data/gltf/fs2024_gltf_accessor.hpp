#pragma once

#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <nlohmann/json_fwd.hpp>
#include <vector>

namespace sdl3cpp::fs2024 {

/// Reads a glTF accessor's raw bytes: `bin[bufferView.byteOffset +
/// accessor.byteOffset + i*stride]`, `stride` from the bufferView's
/// own `byteStride` if it declares one (interleaved attributes),
/// else the tightly-packed size for `componentType`/`type`. FS2024's
/// own accessors are always float VEC3/VEC2 for position/normal/uv,
/// so only those two readers exist; indices vary in width.
std::vector<glm::vec3> ReadFloat3Accessor(const nlohmann::json& gltf,
                                         const std::uint8_t* bin,
                                         int accessorIndex);
std::vector<glm::vec2> ReadFloat2Accessor(const nlohmann::json& gltf,
                                         const std::uint8_t* bin,
                                         int accessorIndex);

/// Reads an index accessor, promoting UNSIGNED_BYTE/SHORT to u32.
std::vector<std::uint32_t> ReadIndexAccessor(const nlohmann::json& gltf,
                                            const std::uint8_t* bin,
                                            int accessorIndex);

}  // namespace sdl3cpp::fs2024
