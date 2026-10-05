#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/racer/data/racer_texture.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

namespace sdl3cpp::services::impl {

/// What one export pass wrote, for the workflow to publish.
struct RacerExportCounts {
    int textures = 0;
    int images = 0;
    int audio = 0;
};

/// Settings shared by the three export passes.
struct RacerExportOptions {
    std::filesystem::path racerDir;
    std::filesystem::path outDir;
    int scale = 4;
    std::uint32_t audioRate = 44100;
};

/// Decodes lev01's texture block and writes each texture upscaled.
int ExportRacerTextures(const RacerExportOptions& options,
                        const std::shared_ptr<ILogger>& logger);

/// Upscales every UI TGA under data/images to PNG.
int ExportRacerImages(const RacerExportOptions& options,
                      const std::shared_ptr<ILogger>& logger);

/// Resamples every WAV under data/wavs to the target rate.
int ExportRacerAudio(const RacerExportOptions& options,
                     const std::shared_ptr<ILogger>& logger);

/// Plots every track-length spline in lev01 as a top-down PNG.
int ExportRacerTracks(const RacerExportOptions& options,
                      const std::shared_ptr<ILogger>& logger);

/// Writes every texture side by side in one PNG, for a quick visual check.
bool WriteRacerTextureSheet(const std::vector<RacerTexture>& textures,
                            const std::filesystem::path& path);

}  // namespace sdl3cpp::services::impl
