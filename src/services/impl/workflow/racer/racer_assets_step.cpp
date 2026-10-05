#include "services/interfaces/workflow/racer/racer_assets_step.hpp"

#include "services/interfaces/workflow/racer/data/racer_asset_export.hpp"
#include "services/interfaces/workflow/racer/racer_step_params.hpp"

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

bool IsPowerOfTwo(int value) {
    return value >= 1 && (value & (value - 1)) == 0;
}

}  // namespace

WorkflowRacerAssetsStep::WorkflowRacerAssetsStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowRacerAssetsStep::GetPluginId() const {
    return "racer.assets.build";
}

void WorkflowRacerAssetsStep::Execute(const WorkflowStepDefinition& step,
                                      WorkflowContext& context) {
    RacerExportOptions options;
    options.racerDir = RacerStringParam(step, "racer_dir", "RACER_DIR", "");
    options.outDir =
        RacerStringParam(step, "out_dir", nullptr, "racer_generated");
    options.scale = RacerIntParam(step, "scale", 4);
    options.trackTable = RacerStringParam(step, "track_table", nullptr,
                                          options.trackTable);
    options.audioRate =
        static_cast<std::uint32_t>(RacerIntParam(step, "audio_rate", 44100));
    if (options.racerDir.empty()) {
        if (logger_) {
            logger_->Error("racer.assets.build: set RACER_DIR to an Episode I "
                           "Racer install");
        }
        return;
    }
    if (!IsPowerOfTwo(options.scale)) {
        if (logger_) {
            logger_->Error("racer.assets.build: scale must be a power of two");
        }
        return;
    }

    const int textures = ExportRacerTextures(options, logger_);
    const int images = ExportRacerImages(options, logger_);
    const int audio = ExportRacerAudio(options, logger_);
    const int tracks = ExportRacerTracks(options, logger_);
    const int models = ExportRacerModels(options, logger_);
    context.Set("racer.texture_count", textures);
    context.Set("racer.image_count", images);
    context.Set("racer.audio_count", audio);
    context.Set("racer.track_count", tracks);
    context.Set("racer.model_count", models);
}

}  // namespace sdl3cpp::services::impl
