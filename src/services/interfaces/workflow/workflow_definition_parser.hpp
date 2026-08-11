#pragma once

#include "services/interfaces/workflow_definition.hpp"
#include "services/interfaces/i_logger.hpp"

#include <rapidjson/document.h>

#include <filesystem>
#include <memory>
#include <unordered_set>
#include <vector>

namespace sdl3cpp::services::impl {

class WorkflowDefinitionParser {
public:
    explicit WorkflowDefinitionParser(std::shared_ptr<ILogger> logger = nullptr);

    WorkflowDefinition ParseFile(const std::filesystem::path& path) const;

private:
    void ParseVariables(const rapidjson::Document& document,
                        WorkflowDefinition& workflow) const;

    std::vector<WorkflowStepDefinition> ParseNodes(
        const rapidjson::Document& document) const;

    // Expand any workflow.include steps in-place (parse-time composition).
    // baseDir  — directory of the including file, for relative path resolution.
    // visited  — canonical paths already on the include stack (cycle detection).
    void ResolveIncludes(std::vector<WorkflowStepDefinition>& steps,
                         const std::filesystem::path& baseDir,
                         std::unordered_set<std::string>& visited) const;

    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
