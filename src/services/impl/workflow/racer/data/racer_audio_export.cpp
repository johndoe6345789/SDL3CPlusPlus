#include "services/interfaces/workflow/racer/data/racer_asset_export.hpp"

#include "services/interfaces/workflow/racer/data/racer_asset_io.hpp"
#include "services/interfaces/workflow/racer/data/racer_wav.hpp"

#include <fstream>
#include <string>
#include <system_error>

namespace sdl3cpp::services::impl {

int ExportRacerAudio(const RacerExportOptions& options,
                     const std::shared_ptr<ILogger>& logger) {
    int written = 0;
    const auto root = options.racerDir / "data" / "wavs";
    for (const auto& entry : ListRacerFiles(root, true)) {
        if (entry.extension() != ".wav" && entry.extension() != ".WAV") {
            continue;
        }
        const auto source = ReadRacerFile(entry);
        RacerPcm pcm;
        if (!source || !ReadRacerWav(*source, pcm)) continue;
        const auto resampled =
            WriteRacerWav(ResampleRacerPcm(pcm, options.audioRate));
        const auto out =
            options.outDir / "wavs" / std::filesystem::relative(entry, root);
        std::error_code ec;
        std::filesystem::create_directories(out.parent_path(), ec);
        std::ofstream file(out, std::ios::binary);
        if (!file) continue;
        file.write(reinterpret_cast<const char*>(resampled.data()),
                   static_cast<std::streamsize>(resampled.size()));
        ++written;
    }
    if (logger) {
        logger->Info("racer: wrote " + std::to_string(written) + " sounds");
    }
    return written;
}

}  // namespace sdl3cpp::services::impl
