#pragma once

#include "services/interfaces/workflow_definition.hpp"

#include <filesystem>

namespace sdl3cpp::services::impl {

class WorkflowDefinitionParser {
public:
    WorkflowDefinition ParseFile(const std::filesystem::path& path) const;
};

}  // namespace sdl3cpp::services::impl
