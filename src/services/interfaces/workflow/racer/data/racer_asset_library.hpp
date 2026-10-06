#pragma once

#include "services/interfaces/workflow/racer/data/racer_block.hpp"
#include "services/interfaces/workflow/racer/data/racer_model.hpp"
#include "services/interfaces/workflow/racer/data/racer_texture.hpp"

#include <filesystem>
#include <optional>
#include <vector>

namespace sdl3cpp::services::impl {

/// The lev01 blocks of one install, read once and indexed.
struct RacerAssetLibrary {
    std::vector<std::uint8_t> modelBlock;
    std::vector<std::uint8_t> textureBlock;
    std::vector<std::uint8_t> splineBlock;
    std::vector<std::vector<RacerBlockPart>> models;
    std::vector<std::vector<RacerBlockPart>> textures;
    std::vector<std::vector<RacerBlockPart>> splines;
    bool valid = false;
};

/// Reads `<racerDir>/data/lev01/out_{model,texture,spline}block.bin`.
RacerAssetLibrary OpenRacerAssetLibrary(const std::filesystem::path& dir);

/// The model at `index`, flattened. Empty if the index or data is bad.
RacerModel LoadRacerModel(const RacerAssetLibrary& library, int index,
                          RacerModelScope scope = RacerModelScope::Everything);

/// The spline item at `index` as raw bytes.
std::vector<std::uint8_t> RacerSplineBytes(const RacerAssetLibrary& library,
                                           int index);

/// The texture a material samples, mirrored to its effective size when
/// the material says so. Empty when the material has no texture.
RacerTexture DecodeRacerMaterialTexture(const RacerAssetLibrary& library,
                                        const RacerMaterialRef& material);

/// Mirrors an image to double width and/or height (right half and
/// bottom half are reflections), the way the N64 mirror-repeat mode
/// samples it.
RacerTexture MirrorRacerTexture(const RacerTexture& texture,
                                bool doubleWidth, bool doubleHeight);

}  // namespace sdl3cpp::services::impl
