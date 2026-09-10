#pragma once

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

}  // namespace sdl3cpp::q3
