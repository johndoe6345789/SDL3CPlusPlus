#pragma once

#include <array>
#include <cstdint>

namespace sdl3cpp::services::impl {

/// A 5 x 7 glyph: seven rows, top first, bit 4 the left column.
using Gta5Glyph = std::array<std::uint8_t, 7>;

/// The map's lettering: A-Z (either case), 0-9, '-', '\'' and '.'.
/// Anything else, space included, is blank.
const Gta5Glyph& Gta5MapGlyph(char c);

}  // namespace sdl3cpp::services::impl
