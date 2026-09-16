#include "services/interfaces/workflow/graphics/video_step_params.hpp"

#include <cstdlib>

namespace sdl3cpp::services::impl {
namespace {

using Type = WorkflowParameterValue::Type;

const WorkflowParameterValue* Find(const WorkflowStepDefinition& step,
                                   const char* name, Type type) {
    const auto it = step.parameters.find(name);
    if (it == step.parameters.end() || it->second.type != type) {
        return nullptr;
    }
    return &it->second;
}

const char* Env(const char* env) {
    const char* text = env ? std::getenv(env) : nullptr;
    return text && *text ? text : nullptr;
}

}  // namespace

double VideoStepNumber(const WorkflowStepDefinition& step, const char* name,
                       const char* env, double fallback) {
    if (const char* text = Env(env)) return std::atof(text);
    const auto* value = Find(step, name, Type::Number);
    return value ? value->numberValue : fallback;
}

std::string VideoStepString(const WorkflowStepDefinition& step,
                            const char* name, const char* env,
                            const std::string& fallback) {
    if (const char* text = Env(env)) return text;
    const auto* value = Find(step, name, Type::String);
    return value ? value->stringValue : fallback;
}

bool VideoStepFlag(const WorkflowStepDefinition& step, const char* name,
                   const char* env, bool fallback) {
    const auto* flag = Find(step, name, Type::Bool);
    const bool on    = flag ? flag->boolValue : fallback;
    return VideoStepNumber(step, name, env, on ? 1.0 : 0.0) != 0.0;
}

}  // namespace sdl3cpp::services::impl
