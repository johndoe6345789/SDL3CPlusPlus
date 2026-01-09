#pragma once

#include <string>
#include <unordered_map>

namespace sdl3cpp::services {

struct WorkflowStepDefinition {
    std::string id;
    std::string plugin;
    std::unordered_map<std::string, std::string> inputs;
    std::unordered_map<std::string, std::string> outputs;
};

}  // namespace sdl3cpp::services
