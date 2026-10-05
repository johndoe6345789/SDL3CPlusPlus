#include "services/interfaces/workflow/racer/data/racer_asset_export.hpp"

#include "services/interfaces/workflow/racer/data/racer_asset_io.hpp"
#include "services/interfaces/workflow/racer/data/racer_texture.hpp"
#include "services/interfaces/workflow/racer/data/racer_upscale.hpp"

#include <string>

namespace sdl3cpp::services::impl {
namespace {

std::vector<std::uint8_t> Scaled(const std::vector<std::uint8_t>& rgba,
                                 int width, int height, int scale) {
    if (scale < 2) return rgba;
    return UpscaleRgba(rgba, width, height, scale);
}

}  // namespace

int ExportRacerTextures(const RacerExportOptions& options,
                        const std::shared_ptr<ILogger>& logger) {
    const auto block = ReadRacerFile(options.racerDir / "data" / "lev01" /
                                     "out_textureblock.bin");
    if (!block) {
        if (logger) logger->Error("racer: texture block not found");
        return 0;
    }
    int written = 0;
    const auto textures = DecodeRacerTextures(*block);
    WriteRacerTextureSheet(textures, options.outDir / "textures" /
                                         "sheet.png");
    for (const RacerTexture& texture : textures) {
        const int size = options.scale < 2 ? 1 : options.scale;
        const int width = texture.width * size;
        const int height = texture.height * size;
        const auto pixels = Scaled(texture.rgba, texture.width,
                                   texture.height, options.scale);
        const auto name = "tex_" + std::to_string(texture.blockIndex) + ".png";
        if (WriteRacerPng(options.outDir / "textures" / name, width, height,
                          pixels)) {
            ++written;
        }
    }
    if (logger) {
        logger->Info("racer: wrote " + std::to_string(written) + " textures");
    }
    return written;
}

int ExportRacerImages(const RacerExportOptions& options,
                      const std::shared_ptr<ILogger>& logger) {
    int written = 0;
    const auto dir = options.racerDir / "data" / "images";
    for (const auto& entry : ListRacerFiles(dir, false)) {
        const auto ext = entry.extension().string();
        if (ext != ".tga" && ext != ".TGA") continue;
        const auto image = ReadRacerImage(entry);
        if (!image) {
            if (logger) logger->Error("racer: cannot read " + entry.string());
            continue;
        }
        const auto pixels =
            Scaled(image->rgba, image->width, image->height, options.scale);
        const int size = options.scale < 2 ? 1 : options.scale;
        const auto out =
            options.outDir / "images" / (entry.stem().string() + ".png");
        if (WriteRacerPng(out, image->width * size, image->height * size,
                          pixels)) {
            ++written;
        }
    }
    if (logger) {
        logger->Info("racer: wrote " + std::to_string(written) + " images");
    }
    return written;
}

}  // namespace sdl3cpp::services::impl
