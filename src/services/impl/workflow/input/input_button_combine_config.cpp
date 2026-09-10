#include "services/interfaces/workflow/input/input_button_combine_config.hpp"

#include <fstream>
#include <stdexcept>

namespace sdl3cpp::services::impl {

nlohmann::json LoadButtonAggregationConfig(const WorkflowContext& context,
                                           const std::string& configPath) {
    const auto* contextConfig =
        context.TryGet<nlohmann::json>("input.aggregation.config");
    if (contextConfig && contextConfig->is_object()) {
        return *contextConfig;
    }

    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        throw std::runtime_error(
            "input.button.combine: Failed to open config: " + configPath);
    }
    nlohmann::json aggregationConfig;
    configFile >> aggregationConfig;
    return aggregationConfig;
}

}  // namespace sdl3cpp::services::impl
