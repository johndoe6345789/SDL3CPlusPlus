#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::fs2024 {

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

/// One LOD's name and size, enough to choose between them without
/// decoding any: FS2024's own LOD0 of Westminster Bridge alone decodes
/// to hundreds of megabytes of vertices.
struct GltfLodInfo {
    std::string modelFile;
    float minSize = 0.f;
    std::size_t bytes = 0;  ///< its binary glTF, a proxy for its detail
};

std::vector<GltfLodInfo> ListModelRiffLods(
    const std::vector<std::uint8_t>& riff);

/// Decodes only LOD `index`.
GltfLod ParseModelRiffLod(const std::vector<std::uint8_t>& riff,
                          std::size_t index);

}  // namespace sdl3cpp::fs2024
