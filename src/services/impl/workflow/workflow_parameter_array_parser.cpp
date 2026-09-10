#include "services/interfaces/workflow/workflow_parameter_array_parser.hpp"
#include "services/interfaces/workflow/workflow_parameter_value_parser.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowParameterValue ParseParameterArrayValue(const rapidjson::Value& value,
                                                const std::string& key) {
    std::vector<std::string> stringItems;
    std::vector<double> numberItems;
    for (rapidjson::SizeType i = 0; i < value.Size(); ++i) {
        const auto& entry = value[i];
        if (entry.IsString()) {
            stringItems.emplace_back(ExpandEnvPlaceholders(entry.GetString()));
        } else if (entry.IsNumber()) {
            numberItems.emplace_back(entry.GetDouble());
        } else {
            throw std::runtime_error("Workflow parameter '" + key +
                                     "' array must contain strings or numbers");
        }
    }
    if (!stringItems.empty() && !numberItems.empty()) {
        throw std::runtime_error("Workflow parameter '" + key +
                                 "' cannot mix string and number values");
    }
    if (!stringItems.empty() || value.Empty()) {
        return WorkflowParameterValue::FromStringList(std::move(stringItems));
    }
    return WorkflowParameterValue::FromNumberList(std::move(numberItems));
}

}  // namespace sdl3cpp::services::impl
