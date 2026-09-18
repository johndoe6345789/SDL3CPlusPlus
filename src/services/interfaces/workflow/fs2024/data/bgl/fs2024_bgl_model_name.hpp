#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::fs2024 {

/// A model directory entry's 16-byte GUID, hex-encoded (lowercase, no
/// braces or dashes) -- just enough to tell entries apart in a log.
std::string HexGuid(const std::uint8_t* bytes16);

/// The `name="..."` attribute of a model's `<ModelInfo>` element, from
/// `riffHead` -- a prefix of its RIFF blob long enough to contain the
/// whole `GXML` chunk (the geometry chunk after it can be huge, so
/// callers peek rather than reading the full blob just to name it).
/// Empty if `riffHead` does not hold a complete GXML chunk.
std::string NameFromGxmlPeek(const std::vector<std::uint8_t>& riffHead);

}  // namespace sdl3cpp::fs2024
