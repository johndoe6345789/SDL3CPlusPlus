#include "services/interfaces/workflow/racer/data/racer_asset_library.hpp"

#include "services/interfaces/workflow/racer/data/racer_asset_io.hpp"

namespace sdl3cpp::services::impl {

RacerAssetLibrary OpenRacerAssetLibrary(const std::filesystem::path& dir) {
    RacerAssetLibrary library;
    const auto lev = dir / "data" / "lev01";
    auto models = ReadRacerFile(lev / "out_modelblock.bin");
    auto textures = ReadRacerFile(lev / "out_textureblock.bin");
    auto splines = ReadRacerFile(lev / "out_splineblock.bin");
    if (!models || !textures || !splines) return library;
    library.modelBlock = std::move(*models);
    library.textureBlock = std::move(*textures);
    library.splineBlock = std::move(*splines);
    library.models = ReadRacerBlock(library.modelBlock, kRacerModelParts);
    library.textures =
        ReadRacerBlock(library.textureBlock, kRacerTextureParts);
    library.splines = ReadRacerBlock(library.splineBlock, kRacerSplineParts);
    library.valid = !library.models.empty() && !library.textures.empty() &&
                    !library.splines.empty();
    return library;
}

RacerModel LoadRacerModel(const RacerAssetLibrary& library, int index) {
    if (index < 0 || index >= static_cast<int>(library.models.size())) {
        return {};
    }
    // Part 0 is the relocation mask the game uses to patch pointers;
    // the parser reads offsets directly, so only part 1 is needed.
    const auto& parts = library.models[index];
    return ParseRacerModel(RacerPartBytes(library.modelBlock, parts[1]));
}

std::vector<std::uint8_t> RacerSplineBytes(const RacerAssetLibrary& library,
                                           int index) {
    if (index < 0 || index >= static_cast<int>(library.splines.size())) {
        return {};
    }
    return RacerPartBytes(library.splineBlock, library.splines[index][0]);
}

RacerTexture DecodeRacerMaterialTexture(const RacerAssetLibrary& library,
                                        const RacerMaterialRef& material) {
    const int index = material.textureIndex;
    if (index < 0 || index >= static_cast<int>(library.textures.size())) {
        return {};
    }
    const auto& parts = library.textures[index];
    const RacerTexture texture = DecodeRacerTexture(
        RacerPartBytes(library.textureBlock, parts[0]),
        RacerPartBytes(library.textureBlock, parts[1]), material.format,
        material.width, material.height);
    return MirrorRacerTexture(texture, material.doubleWidth,
                              material.doubleHeight);
}

}  // namespace sdl3cpp::services::impl
