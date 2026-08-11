#include "services/interfaces/workflow/workflow_generic_steps/workflow_vfx_destroy_step.hpp"

#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <utility>
#include <string>
#include <vector>
#include <algorithm>

namespace sdl3cpp::services::impl {

WorkflowVfxDestroyStep::WorkflowVfxDestroyStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowVfxDestroyStep::GetPluginId() const {
    return "vfx.destroy";
}

void WorkflowVfxDestroyStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepParameterResolver parameterResolver;
    WorkflowStepIoResolver ioResolver;

    // Get active VFX list
    std::vector<std::string> effects;
    if (const auto* existingEffects = context.TryGet<std::vector<std::string>>("vfx.active")) {
        effects = *existingEffects;
    }

    bool destroyed = false;

    // Check for destroy_all flag
    if (const auto* param = parameterResolver.FindParameter(step, "destroy_all")) {
        if (param->type == WorkflowParameterValue::Type::Bool && param->boolValue) {
            effects.clear();
            destroyed = true;
        }
    }

    // Check for vfx_id parameter
    if (!destroyed) {
        if (const auto* param = parameterResolver.FindParameter(step, "vfx_id")) {
            if (param->type == WorkflowParameterValue::Type::String && !param->stringValue.empty()) {
                auto it = std::find(effects.begin(), effects.end(), param->stringValue);
                if (it != effects.end()) {
                    effects.erase(it);
                    destroyed = true;
                }
            }
        }
    }

    // Check for vfx_ids (comma-separated)
    if (!destroyed) {
        if (const auto* param = parameterResolver.FindParameter(step, "vfx_ids")) {
            if (param->type == WorkflowParameterValue::Type::String && !param->stringValue.empty()) {
                // Parse comma-separated IDs
                std::string idStr = param->stringValue;
                size_t pos = 0;
                while (pos < idStr.length()) {
                    size_t commaPos = idStr.find(',', pos);
                    std::string id = idStr.substr(pos,
                        commaPos == std::string::npos ? std::string::npos : commaPos - pos);

                    // Trim whitespace
                    size_t start = id.find_first_not_of(" \t");
                    if (start != std::string::npos) {
                        id = id.substr(start);
                    }

                    if (!id.empty()) {
                        auto it = std::find(effects.begin(), effects.end(), id);
                        if (it != effects.end()) {
                            effects.erase(it);
                            destroyed = true;
                        }
                    }

                    if (commaPos == std::string::npos) break;
                    pos = commaPos + 1;
                }
            }
        }
    }

    // Check for target (oldest/newest)
    if (!destroyed && !effects.empty()) {
        if (const auto* param = parameterResolver.FindParameter(step, "target")) {
            if (param->type == WorkflowParameterValue::Type::String) {
                if (param->stringValue == "oldest") {
                    effects.erase(effects.begin());
                    destroyed = true;
                } else if (param->stringValue == "newest") {
                    effects.pop_back();
                    destroyed = true;
                }
            }
        }
    }

    // Store back to context
    context.Set("vfx.active", effects);

    // Store output values
    const auto destroyedOutputIt = step.outputs.find("destroyed");
    if (destroyedOutputIt != step.outputs.end()) {
        context.Set(destroyedOutputIt->second, destroyed);
    }

    const auto countOutputIt = step.outputs.find("remaining_count");
    if (countOutputIt != step.outputs.end()) {
        context.Set(countOutputIt->second, static_cast<double>(effects.size()));
    }

    if (logger_) {
        logger_->Trace("WorkflowVfxDestroyStep", "Execute",
                       "destroyed=" + std::string(destroyed ? "true" : "false") +
                       ", remaining=" + std::to_string(effects.size()),
                       "VFX destruction complete");
    }
}

}  // namespace sdl3cpp::services::impl
