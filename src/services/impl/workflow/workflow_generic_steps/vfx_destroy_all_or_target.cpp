#include "services/interfaces/workflow/workflow_generic_steps/vfx_destroy_all_or_target.hpp"

namespace sdl3cpp::services::impl {

bool DestroyAll(const WorkflowStepDefinition& step,
                const WorkflowStepParameterResolver& resolver,
                std::vector<std::string>& effects) {
    const auto* param = resolver.FindParameter(step, "destroy_all");
    if (!param || param->type != WorkflowParameterValue::Type::Bool ||
        !param->boolValue) {
        return false;
    }
    effects.clear();
    return true;
}

bool DestroyByTarget(const WorkflowStepDefinition& step,
                     const WorkflowStepParameterResolver& resolver,
                     std::vector<std::string>& effects) {
    if (effects.empty()) return false;
    const auto* param = resolver.FindParameter(step, "target");
    if (!param || param->type != WorkflowParameterValue::Type::String) {
        return false;
    }
    if (param->stringValue == "oldest") {
        effects.erase(effects.begin());
        return true;
    }
    if (param->stringValue == "newest") {
        effects.pop_back();
        return true;
    }
    return false;
}

}  // namespace sdl3cpp::services::impl
