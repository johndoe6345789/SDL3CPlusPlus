#include "services/interfaces/workflow/input/input_aggregation_config.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <fstream>
#include <stdexcept>

namespace sdl3cpp::services::impl {

nlohmann::json LoadInputAggregationConfig(const WorkflowStepDefinition& step,
                                          const WorkflowContext& context) {
    WorkflowStepParameterResolver paramResolver;
    std::string configPath = "packages/seed/workflows/input_aggregation.json";
    if (const auto* param = paramResolver.FindParameter(step, "config_path")) {
        if (param->type == WorkflowParameterValue::Type::String) {
            configPath = param->stringValue;
        }
    }

    const auto* contextConfig =
        context.TryGet<nlohmann::json>("input.aggregation.config");
    if (contextConfig && contextConfig->is_object()) {
        return *contextConfig;
    }

    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        throw std::runtime_error("input.axis.combine: Failed to open config: " +
                                 configPath);
    }
    nlohmann::json aggregationConfig;
    configFile >> aggregationConfig;
    return aggregationConfig;
}

}  // namespace sdl3cpp::services::impl
