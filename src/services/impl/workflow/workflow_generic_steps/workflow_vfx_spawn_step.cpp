#include "services/interfaces/workflow/workflow_generic_steps/workflow_vfx_spawn_step.hpp"

#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <utility>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>

namespace sdl3cpp::services::impl {

WorkflowVfxSpawnStep::WorkflowVfxSpawnStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowVfxSpawnStep::GetPluginId() const {
    return "vfx.spawn";
}

void WorkflowVfxSpawnStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepParameterResolver parameterResolver;
    WorkflowStepIoResolver ioResolver;

    // Get effect type
    std::string effectType = "default";
    if (const auto* param = parameterResolver.FindParameter(step, "effect_type")) {
        if (param->type == WorkflowParameterValue::Type::String) {
            effectType = param->stringValue;
        }
    }

    // Generate unique VFX ID
    std::ostringstream oss;
    oss << "vfx_" << std::setfill('0') << std::setw(3) << nextVfxId_;
    std::string vfxId = oss.str();
    ++nextVfxId_;

    // Get or create active VFX list, add new effect
    auto* existing = context.TryGet<std::vector<std::string>>("vfx.active");
    std::vector<std::string> effects = existing ? *existing : std::vector<std::string>{};
    effects.push_back(vfxId);
    context.Set("vfx.active", effects);

    // Store effect info if output is requested
    const auto outputIt = step.outputs.find("vfx_id");
    if (outputIt != step.outputs.end()) {
        context.Set(outputIt->second, vfxId);
    }

    if (logger_) {
        logger_->Trace("WorkflowVfxSpawnStep", "Execute",
                       "type=" + effectType + ", id=" + vfxId,
                       "Spawned VFX");
    }
}

}  // namespace sdl3cpp::services::impl
