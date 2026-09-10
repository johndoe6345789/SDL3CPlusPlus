#include "services/interfaces/workflow/rendering/model_load_params.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {

ModelLoadParams ReadModelLoadParams(const WorkflowStepDefinition& step,
                                    const WorkflowContext& context) {
    WorkflowStepParameterResolver params;
    auto getStr = [&](const char* name, const std::string& def) -> std::string {
        const auto* p = params.FindParameter(step, name);
        if (p && p->type == WorkflowParameterValue::Type::String) {
            return p->stringValue;
        }
        auto it = step.inputs.find(name);
        if (it != step.inputs.end()) {
            const auto* ctx = context.TryGet<std::string>(it->second);
            if (ctx) return *ctx;
        }
        return def;
    };
    auto getNum = [&](const char* name, float def) -> float {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number)
                   ? static_cast<float>(p->numberValue)
                   : def;
    };

    ModelLoadParams out;
    out.filePath = getStr("file_path", "");
    out.name     = getStr("name", out.name);
    out.scale    = getNum("scale", out.scale);
    return out;
}

}  // namespace sdl3cpp::services::impl
