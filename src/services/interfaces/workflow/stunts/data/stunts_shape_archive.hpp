#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief The archive a decoded .P3S/.PVS file opens with.
 *
 * After the container and run-length passes, a shape file is: a
 * 32-bit total size, a 16-bit entry count, that many four-character
 * tags, then that many 32-bit offsets (from the byte right after the
 * offset table) marking where each tag's shape data starts. A shape
 * runs to the next entry's offset, or to the end of the file for the
 * last one.
 */
using StuntsShapeArchive = std::map<std::string, std::vector<std::uint8_t>>;

/// Splits a decoded .P3S/.PVS buffer into its named shapes.
StuntsShapeArchive ParseStuntsShapeArchive(
    const std::vector<std::uint8_t>& decoded);

}  // namespace sdl3cpp::services::impl
