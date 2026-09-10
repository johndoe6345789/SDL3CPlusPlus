#include "services/interfaces/workflow/rendering/map_load_params.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {

MapLoadParams ReadMapLoadParams(const WorkflowStepDefinition& step,
                                const WorkflowContext& context) {
    WorkflowStepParameterResolver params;

    auto getStr = [&](const char* name, const std::string& def) {
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

    MapLoadParams result;
    result.filePath      = getStr("file_path", "");
    result.scale         = getNum("scale", 1.0f);
    result.createPhysics = static_cast<int>(getNum("create_physics", 1)) != 0;
    return result;
}

}  // namespace sdl3cpp::services::impl
