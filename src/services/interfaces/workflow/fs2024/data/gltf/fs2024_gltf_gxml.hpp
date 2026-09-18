#pragma once

#include <string>
#include <vector>

namespace sdl3cpp::fs2024 {

/// One LOD as a model's `GXML` chunk lists it.
struct GltfLodHeader {
    std::string modelFile;  ///< e.g. "WestminsterPalace_LOD0.gltf"
    float minSize = 0.f;
};

/// Every `<LOD ...>` of a model's GXML, in order.
std::vector<GltfLodHeader> ParseGxmlLods(const std::string& xml);

}  // namespace sdl3cpp::fs2024
