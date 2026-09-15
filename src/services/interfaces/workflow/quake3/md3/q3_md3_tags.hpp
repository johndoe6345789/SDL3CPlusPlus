#pragma once

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <stdint.h>
#include <vector>

namespace sdl3cpp::q3 {

/**
 * @brief Converts an MD3's per-frame tags into engine-space JSON.
 *
 * Quake 3 is Z-up and the engine is Y-up, so origins and basis vectors are
 * remapped as x'=x, y'=z, z'=-y.  Origins are additionally scaled by
 * kMd3XyzScale * kMd3WorldScale so tags line up with the loaded BSP.
 *
 * @param md3Bytes Whole MD3 file; a truncated or tag-less file yields [].
 * @return Array indexed by frame, each an object of tag name -> {origin, axis}.
 */
nlohmann::json BuildMd3TagsJson(const std::vector<uint8_t>& md3Bytes);

/**
 * @brief Builds a glm matrix from one engine-space tag (as produced by
 * BuildMd3TagsJson): {"origin": [x,y,z], "axis": [[x,y,z] x 3]}.
 *
 * The tag's axis rows (already coord-converted to engine space) must be
 * transposed to form the columns of a column-major glm matrix.
 */
glm::mat4 TagMatrix(const nlohmann::json& tag);

}  // namespace sdl3cpp::q3
