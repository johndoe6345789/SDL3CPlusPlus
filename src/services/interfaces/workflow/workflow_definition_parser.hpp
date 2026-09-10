#pragma once

#include "services/interfaces/workflow_definition.hpp"
#include "services/interfaces/workflow/workflow_parameter_reader.hpp"
#include "services/interfaces/i_logger.hpp"

#include <rapidjson/document.h>

#include <filesystem>
#include <memory>
#include <unordered_set>
#include <vector>

namespace sdl3cpp::services::impl {

class WorkflowDefinitionParser {
public:
    explicit WorkflowDefinitionParser(
        std::shared_ptr<ILogger> logger = nullptr);

    WorkflowDefinition ParseFile(const std::filesystem::path& path) const;

private:
    void ParseVariables(const rapidjson::Document& document,
                        WorkflowDefinition& workflow) const;

    // Handles the simple sequential "steps" array format.
    // Throws if `document["steps"]` isn't an array or a step isn't an object.
    std::vector<WorkflowStepDefinition> ParseStepsFormat(
        const rapidjson::Document& document,
        const WorkflowParameterReader& paramReader) const;

    std::vector<WorkflowStepDefinition> ParseNodes(
        const rapidjson::Document& document) const;

    // Expand any workflow.include steps in-place (parse-time composition).
    // baseDir  — directory of the including file, for relative path
    //            resolution.
    // visited  — canonical paths already on the include stack (cycle
    //            detection).
    void ResolveIncludes(std::vector<WorkflowStepDefinition>& steps,
                         const std::filesystem::path& baseDir,
                         std::unordered_set<std::string>& visited) const;

    // Expands one workflow.include step, appending the resulting steps
    // (namespaced under `step.id`/) to `expanded`. A no-op if `step` isn't
    // a workflow.include step or its "path" parameter is missing.
    void ExpandInclude(WorkflowStepDefinition& step,
                       const std::filesystem::path& baseDir,
                       std::unordered_set<std::string>& visited,
                       std::vector<WorkflowStepDefinition>& expanded) const;

    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
