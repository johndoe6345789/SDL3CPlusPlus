#pragma once

#include <vector>

namespace sdl3cpp::services::impl {

/// A square grid of terrain heights in metres, row-major from the lowest z.
struct SwitchbackHeightmap {
    int size = 0;
    std::vector<float> metres;
};

}  // namespace sdl3cpp::services::impl
