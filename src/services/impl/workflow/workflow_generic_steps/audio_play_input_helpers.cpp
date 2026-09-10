#include "services/interfaces/workflow/workflow_generic_steps/audio_play_input_helpers.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace sdl3cpp::services::impl {

std::string ResolveAudioPlayMode(
    const WorkflowStepDefinition& step, const WorkflowContext& context,
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
            throw std::runtime_error(
                "audio.play parameter 'mode' must be a string");
        }
        return param->stringValue;
    }
    return "effect";
}

bool ResolveAudioPlayLoop(
    const WorkflowStepDefinition& step, const WorkflowContext& context,
    const WorkflowStepParameterResolver& parameterResolver, bool fallback) {
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
            throw std::runtime_error(
                "audio.play parameter 'loop' must be a bool");
        }
        return param->boolValue;
    }
    return fallback;
}

std::string NormalizeAudioPlayMode(std::string mode) {
    std::transform(mode.begin(), mode.end(), mode.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });
    return mode;
}

}  // namespace sdl3cpp::services::impl
