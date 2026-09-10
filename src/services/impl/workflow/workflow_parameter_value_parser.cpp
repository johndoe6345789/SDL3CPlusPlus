#include "services/interfaces/workflow/workflow_parameter_value_parser.hpp"
#include "services/interfaces/workflow/workflow_parameter_array_parser.hpp"

#include <cstdlib>
#include <stdexcept>

namespace sdl3cpp::services::impl {

std::string ExpandEnvPlaceholders(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    size_t i = 0;
    while (i < input.size()) {
        const size_t open = input.find("${env:", i);
        if (open == std::string::npos) {
            out.append(input, i, std::string::npos);
            break;
        }
        const size_t close = input.find('}', open);
        if (close == std::string::npos) {
            out.append(input, i, std::string::npos);
            break;
        }
        out.append(input, i, open - i);
        const std::string varName = input.substr(open + 6, close - (open + 6));
        if (const char* envVal = std::getenv(varName.c_str())) {
            out.append(envVal);
        }
        i = close + 1;
    }
    return out;
}

WorkflowParameterValue ParseParameterValue(const rapidjson::Value& value,
                                           const std::string& key) {
    if (value.IsString()) {
        return WorkflowParameterValue::FromString(
            ExpandEnvPlaceholders(value.GetString()));
    }
    if (value.IsBool()) {
        return WorkflowParameterValue::FromBool(value.GetBool());
    }
    if (value.IsNumber()) {
        return WorkflowParameterValue::FromNumber(value.GetDouble());
    }
    if (value.IsArray()) {
        return ParseParameterArrayValue(value, key);
    }
    throw std::runtime_error("Workflow parameter '" + key +
                             "' must be a string, number, bool, or array");
}

}  // namespace sdl3cpp::services::impl
