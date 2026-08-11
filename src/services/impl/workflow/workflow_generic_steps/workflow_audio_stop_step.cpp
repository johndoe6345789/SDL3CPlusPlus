#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_stop_step.hpp"

#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {
std::string NormalizeMode(std::string mode) {
    std::transform(mode.begin(), mode.end(), mode.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return mode;
}
}  // namespace

WorkflowAudioStopStep::WorkflowAudioStopStep(std::shared_ptr<IAudioService> audioService,
                                             std::shared_ptr<ILogger> logger)
    : audioService_(std::move(audioService)),
      logger_(std::move(logger)) {}

std::string WorkflowAudioStopStep::GetPluginId() const {
    return "audio.stop";
}

void WorkflowAudioStopStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!audioService_) {
        throw std::runtime_error("audio.stop requires an IAudioService");
    }

    WorkflowStepParameterResolver parameterResolver;
    std::string mode = "all";
    auto it = step.inputs.find("mode");
    if (it != step.inputs.end()) {
        const auto* inputMode = context.TryGet<std::string>(it->second);
        if (!inputMode) {
            throw std::runtime_error("audio.stop requires string mode input");
        }
        mode = *inputMode;
    } else if (const auto* param = parameterResolver.FindParameter(step, "mode")) {
        if (param->type != WorkflowParameterValue::Type::String) {
            throw std::runtime_error("audio.stop parameter 'mode' must be a string");
        }
        mode = param->stringValue;
    }
    mode = NormalizeMode(mode);

    if (mode == "background" || mode == "music") {
        audioService_->StopBackground();
    } else if (mode == "all") {
        audioService_->StopAll();
    } else {
        throw std::runtime_error("audio.stop mode must be 'background' or 'all'");
    }

    if (logger_) {
        logger_->Trace("WorkflowAudioStopStep", "Execute",
                       "mode=" + mode,
                       "Stopped audio playback");
    }
}

}  // namespace sdl3cpp::services::impl
