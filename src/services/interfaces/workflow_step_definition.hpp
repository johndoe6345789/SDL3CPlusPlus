#pragma once

#include "services/interfaces/workflow_parameter_value.hpp"

#include <string>
#include <unordered_map>

namespace sdl3cpp::services {

struct WorkflowStepDefinition {
    std::string id;
    std::string name;
    std::string plugin;
    std::unordered_map<std::string, std::string> inputs;
    std::unordered_map<std::string, std::string> outputs;
    std::unordered_map<std::string, WorkflowParameterValue> parameters;
};

}  // namespace sdl3cpp::services
