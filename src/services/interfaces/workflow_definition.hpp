#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <string>
#include <vector>
#include <unordered_map>

namespace sdl3cpp::services {

struct WorkflowVariable {
    std::string name;
    std::string type;           // string, number, boolean, array, object, date, any
    std::string description;
    std::string defaultValue;   // Stored as string, converted based on type
    bool required = false;
};

struct WorkflowDefinition {
    std::string templateName;
    std::unordered_map<std::string, WorkflowVariable> variables;  // N8N-style workflow variables
    std::unordered_map<std::string, std::string> cliArgs;        // CLI arguments accessible via {{ $cli.* }}
    std::vector<WorkflowStepDefinition> steps;
};

}  // namespace sdl3cpp::services
