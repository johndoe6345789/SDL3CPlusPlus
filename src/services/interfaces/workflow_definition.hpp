#pragma once

#include "workflow_step_definition.hpp"

#include <string>
#include <vector>

namespace sdl3cpp::services {

struct WorkflowDefinition {
    std::string templateName;
    std::vector<WorkflowStepDefinition> steps;
};

}  // namespace sdl3cpp::services
