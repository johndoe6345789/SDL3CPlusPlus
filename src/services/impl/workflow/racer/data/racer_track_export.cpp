#include "services/interfaces/workflow/racer/data/racer_asset_export.hpp"

#include "services/interfaces/workflow/racer/data/racer_asset_io.hpp"
#include "services/interfaces/workflow/racer/data/racer_asset_library.hpp"
#include "services/interfaces/workflow/racer/data/racer_track_plot.hpp"
#include "services/interfaces/workflow/racer/data/racer_track_table.hpp"

#include <string>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kPlotSize = 1024;

}  // namespace

int ExportRacerTracks(const RacerExportOptions& options,
                      const std::shared_ptr<ILogger>& logger) {
    const RacerAssetLibrary library = OpenRacerAssetLibrary(options.racerDir);
    const RacerTrackTable table = LoadRacerTrackTable(options.trackTable);
    if (!library.valid || !table.loaded) {
        if (logger) logger->Error("racer: blocks or track table missing");
        return 0;
    }
    int written = 0;
    for (const RacerTrackInfo& track : table.tracks) {
        const auto segments =
            ReadRacerSpline(RacerSplineBytes(library, track.spline));
        const auto loop = RacerSplineMainLoop(segments);
        if (logger) {
            logger->Info("racer: " + track.name + ": " +
                         std::to_string(segments.size()) + " segments, " +
                         std::to_string(loop.size()) + " on the lap");
        }
        const auto name = "track_" + std::to_string(track.id) + ".png";
        if (WriteRacerPng(options.outDir / "tracks" / name, kPlotSize,
                          kPlotSize, PlotRacerTrack(segments, kPlotSize))) {
            ++written;
        }
    }
    if (logger) {
        logger->Info("racer: wrote " + std::to_string(written) + " tracks");
    }
    return written;
}

}  // namespace sdl3cpp::services::impl
