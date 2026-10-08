#pragma once

#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// A square grid of terrain heights in metres, row-major from the lowest z.
struct SwitchbackHeightmap {
    int size = 0;
    std::vector<float> metres;
};

/// Reads a square little-endian uint16 heightmap (.r16), scaling 0..65535
/// onto 0..heightMaxM. Returns false if the file is missing or not square.
bool ReadSwitchbackHeightmap(const std::string& path, float heightMaxM,
                             SwitchbackHeightmap& out);

}  // namespace sdl3cpp::services::impl
