#include "services/interfaces/workflow/quake3/workflow_q3_music_step.hpp"
#include "services/interfaces/workflow/quake3/q3_music_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {
namespace {
// Keep roughly this many bytes queued so the loop never runs dry between
// frames.
constexpr int kRefillBelowBytes = 1 << 18;
}  // namespace

WorkflowQ3MusicStep::WorkflowQ3MusicStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

WorkflowQ3MusicStep::~WorkflowQ3MusicStep() {
    if (stream_) {
        SDL_UnbindAudioStream(stream_);
        SDL_DestroyAudioStream(stream_);
    }
}

std::string WorkflowQ3MusicStep::GetPluginId() const {
    return "q3.music.play";
}

bool WorkflowQ3MusicStep::Start(const WorkflowStepDefinition& step,
                                WorkflowContext& context) {
    WorkflowStepParameterResolver params;
    auto text = [&](const char* name) -> std::string {
        const auto* p = params.FindParameter(step, name);
        return p ? p->stringValue : std::string();
    };

    const auto device = context.Get<SDL_AudioDeviceID>("q3.sound.device", 0);
    if (device == 0) return false;

    const auto bspConfig =
        context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    const std::string pk3 = bspConfig.value("pk3_path", std::string());
    if (pk3.empty()) return false;

    const auto deviceSpec =
        context.Get<SDL_AudioSpec>("q3.sound.device_spec", SDL_AudioSpec{});
    if (!LoadQ3MusicTracks(pk3, text("intro"), text("loop"), deviceSpec,
                          logger_, stream_, loopPcm_)) {
        return false;
    }

    if (!SDL_BindAudioStream(device, stream_)) {
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
        return false;
    }
    SDL_ResumeAudioDevice(device);
    if (logger_) logger_->Info("q3.music.play: " + text("loop"));
    return true;
}

void WorkflowQ3MusicStep::Execute(const WorkflowStepDefinition& step,
                                  WorkflowContext& context) {
    if (!stream_ && !Start(step, context)) return;
    if (SDL_GetAudioStreamAvailable(stream_) >= kRefillBelowBytes) return;
    SDL_PutAudioStreamData(stream_, loopPcm_.data(),
                           static_cast<int>(loopPcm_.size()));
}

}  // namespace sdl3cpp::services::impl
