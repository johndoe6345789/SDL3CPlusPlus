#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::tools::fs2024 {

/// One glTF mesh primitive, already converted to this engine's own
/// vertex format -- ready for the same upload path terrain/OSM
/// buildings use. `baseColorImageUri` names an image entry from the
/// model's own glTF (e.g. `PALACEOFWESTMINSTER_WALL01_ALBEDO.PNG.DDS`);
/// empty if the primitive's material has none.
struct GltfPrimitive {
    services::impl::TextureGroup mesh;
    std::string baseColorImageUri;
};

/// One level of detail, as named by the model's `GXML` chunk.
struct GltfLod {
    std::string modelFile;  ///< e.g. "WestminsterPalace_LOD0.gltf"
    float minSize = 0.f;
    std::vector<GltfPrimitive> primitives;
};

/// Parses a model's full `RIFF ... GLTF` blob (`ReadModelRiff`'s
/// result) into its LODs, in the order its `GXML` chunk lists them.
std::vector<GltfLod> ParseModelRiff(const std::vector<std::uint8_t>& riff);

}  // namespace sdl3cpp::tools::fs2024
