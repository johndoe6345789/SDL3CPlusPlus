#include "services/interfaces/workflow/racer/data/racer_asset_export.hpp"

#include "services/interfaces/workflow/racer/data/racer_asset_io.hpp"
#include "services/interfaces/workflow/racer/data/racer_upscale.hpp"

#include <string>

namespace sdl3cpp::services::impl {

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
            options.scale < 2
                ? image->rgba
                : UpscaleRgba(image->rgba, image->width, image->height,
                              options.scale);
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
