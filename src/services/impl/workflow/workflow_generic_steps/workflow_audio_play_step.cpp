#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_play_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {
std::string ResolveMode(const WorkflowStepDefinition& step,
                        const WorkflowContext& context,
                        const WorkflowStepParameterResolver& parameterResolver) {
    auto it = step.inputs.find("mode");
    if (it != step.inputs.end()) {
        const auto* mode = context.TryGet<std::string>(it->second);
        if (!mode) {
            throw std::runtime_error("audio.play requires string mode input");
        }
        return *mode;
    }
    if (const auto* param = parameterResolver.FindParameter(step, "mode")) {
        if (param->type != WorkflowParameterValue::Type::String) {
            throw std::runtime_error("audio.play parameter 'mode' must be a string");
        }
        return param->stringValue;
    }
    return "effect";
}

bool ResolveLoop(const WorkflowStepDefinition& step,
                 const WorkflowContext& context,
                 const WorkflowStepParameterResolver& parameterResolver,
                 bool fallback) {
    auto it = step.inputs.find("loop");
    if (it != step.inputs.end()) {
        const auto* loop = context.TryGet<bool>(it->second);
        if (!loop) {
            throw std::runtime_error("audio.play requires bool loop input");
        }
        return *loop;
    }
    if (const auto* param = parameterResolver.FindParameter(step, "loop")) {
        if (param->type != WorkflowParameterValue::Type::Bool) {
            throw std::runtime_error("audio.play parameter 'loop' must be a bool");
        }
        return param->boolValue;
    }
    return fallback;
}

std::string NormalizeMode(std::string mode) {
    std::transform(mode.begin(), mode.end(), mode.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return mode;
}
}  // namespace

WorkflowAudioPlayStep::WorkflowAudioPlayStep(std::shared_ptr<IAudioService> audioService,
                                             std::shared_ptr<ILogger> logger)
    : audioService_(std::move(audioService)),
      logger_(std::move(logger)) {}

std::string WorkflowAudioPlayStep::GetPluginId() const {
    return "audio.play";
}

void WorkflowAudioPlayStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!audioService_) {
        throw std::runtime_error("audio.play requires an IAudioService");
    }

    WorkflowStepIoResolver resolver;
    WorkflowStepParameterResolver parameterResolver;
    const std::string pathKey = resolver.GetRequiredInputKey(step, "path");

    std::filesystem::path pathValue;
    if (const auto* path = context.TryGet<std::filesystem::path>(pathKey)) {
        pathValue = *path;
    } else if (const auto* pathString = context.TryGet<std::string>(pathKey)) {
        pathValue = *pathString;
    } else {
        throw std::runtime_error("audio.play missing path input '" + pathKey + "'");
    }

    std::string mode = NormalizeMode(ResolveMode(step, context, parameterResolver));
    bool loop = ResolveLoop(step, context, parameterResolver, false);

    if (mode == "background" || mode == "music") {
        audioService_->PlayBackground(pathValue, loop);
    } else if (mode == "effect" || mode == "sfx") {
        audioService_->PlayEffect(pathValue, loop);
    } else {
        throw std::runtime_error("audio.play mode must be 'background' or 'effect'");
    }

    if (logger_) {
        logger_->Trace("WorkflowAudioPlayStep", "Execute",
                       "path=" + pathValue.string() +
                           ", mode=" + mode +
                           ", loop=" + std::string(loop ? "true" : "false"),
                       "Dispatched audio playback");
    }
}

}  // namespace sdl3cpp::services::impl
