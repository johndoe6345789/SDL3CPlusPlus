#include "services/interfaces/workflow/quake3/q3_md3_source.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {
namespace {

std::string StringParameter(const WorkflowStepParameterResolver& params,
                            const WorkflowStepDefinition& step, const char* key,
                            const std::string& fallback) {
    const auto* value = params.FindParameter(step, key);
    const bool isString =
        value && value->type == WorkflowParameterValue::Type::String;
    return isString ? value->stringValue : fallback;
}

}  // namespace

Q3Md3StepParameters ReadQ3Md3StepParameters(
    const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;
    Q3Md3StepParameters out;
    out.prefix = StringParameter(params, step, "prefix", "model");
    out.path   = StringParameter(params, step, "path", "");
    out.skin   = StringParameter(params, step, "skin", "");
    out.anim   = StringParameter(params, step, "anim", "");
    return out;
}

std::string Q3Md3SourceKey(const std::string& prefix) {
    return "q3.md3." + prefix + "_source";
}

}  // namespace sdl3cpp::services::impl
