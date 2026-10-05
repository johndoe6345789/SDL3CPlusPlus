#include "services/interfaces/workflow/racer/data/racer_asset_export.hpp"

#include "services/interfaces/workflow/racer/data/racer_asset_io.hpp"
#include "services/interfaces/workflow/racer/data/racer_block_table.hpp"
#include "services/interfaces/workflow/racer/data/racer_spline.hpp"
#include "services/interfaces/workflow/racer/data/racer_track_plot.hpp"

#include <string>

namespace sdl3cpp::services::impl {
namespace {

// Spline entries with fewer records than this are short paths, not tracks.
constexpr std::size_t kMinTrackRecords = 20;
constexpr int kPlotSize = 1024;

}  // namespace

int ExportRacerTracks(const RacerExportOptions& options,
                      const std::shared_ptr<ILogger>& logger) {
    const auto block = ReadRacerFile(options.racerDir / "data" / "lev01" /
                                     "out_splineblock.bin");
    if (!block) {
        if (logger) logger->Error("racer: spline block not found");
        return 0;
    }
    int written = 0;
    for (const RacerBlockEntry& entry : ReadRacerBlockTable(*block)) {
        const auto records = ReadRacerSpline(block->data() + entry.start,
                                             entry.end - entry.start);
        if (records.size() < kMinTrackRecords) continue;
        const auto pixels = PlotRacerTrack(records, kPlotSize);
        const auto name = "spline_" + std::to_string(entry.index) + ".png";
        if (WriteRacerPng(options.outDir / "tracks" / name, kPlotSize,
                          kPlotSize, pixels)) {
            ++written;
        }
    }
    if (logger) {
        logger->Info("racer: wrote " + std::to_string(written) + " tracks");
    }
    return written;
}

}  // namespace sdl3cpp::services::impl
