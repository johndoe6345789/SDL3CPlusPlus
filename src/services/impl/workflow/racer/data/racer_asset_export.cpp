#include "services/interfaces/workflow/racer/data/racer_asset_export.hpp"

#include "services/interfaces/workflow/racer/data/racer_asset_io.hpp"
#include "services/interfaces/workflow/racer/data/racer_asset_library.hpp"
#include "services/interfaces/workflow/racer/data/racer_upscale.hpp"

#include <string>

namespace sdl3cpp::services::impl {
namespace {

std::vector<std::uint8_t> Scaled(const std::vector<std::uint8_t>& rgba,
                                 int width, int height, int scale) {
    if (scale < 2) return rgba;
    return UpscaleRgba(rgba, width, height, scale);
}

/// Every distinct material any model uses. The texture block stores no
/// format or size, so a texture can only be decoded through a material.
std::vector<RacerMaterialRef> AllMaterials(const RacerAssetLibrary& lib) {
    std::vector<RacerMaterialRef> out;
    for (int i = 0; i < static_cast<int>(lib.models.size()); ++i) {
        for (const auto& batch : LoadRacerModel(lib, i).batches) {
            if (batch.material.textureIndex < 0) continue;
            bool seen = false;
            for (const auto& m : out) seen = seen || m == batch.material;
            if (!seen) out.push_back(batch.material);
        }
    }
    return out;
}

}  // namespace

int ExportRacerTextures(const RacerExportOptions& options,
                        const std::shared_ptr<ILogger>& logger) {
    const RacerAssetLibrary library = OpenRacerAssetLibrary(options.racerDir);
    if (!library.valid) {
        if (logger) logger->Error("racer: lev01 blocks not found");
        return 0;
    }
    int written = 0;
    std::vector<RacerTexture> sheet;
    for (const RacerMaterialRef& material : AllMaterials(library)) {
        const RacerTexture t = DecodeRacerMaterialTexture(library, material);
        if (t.rgba.empty()) continue;
        sheet.push_back(t);
        const int size = options.scale < 2 ? 1 : options.scale;
        const auto name = "tex_" + std::to_string(material.textureIndex) +
                          "_" + std::to_string(t.width) + "x" +
                          std::to_string(t.height) + ".png";
        if (WriteRacerPng(options.outDir / "textures" / name,
                          t.width * size, t.height * size,
                          Scaled(t.rgba, t.width, t.height, options.scale))) {
            ++written;
        }
    }
    WriteRacerTextureSheet(sheet, options.outDir / "textures" / "sheet.png");
    if (logger) {
        logger->Info("racer: wrote " + std::to_string(written) + " textures");
    }
    return written;
}

}  // namespace sdl3cpp::services::impl
