#include "services/interfaces/workflow/workflow_execute_helpers.hpp"
#include "services/interfaces/workflow/workflow_definition_parser.hpp"

#include <filesystem>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowDefinition LoadChildWorkflow(const std::shared_ptr<ILogger>& logger,
                                     const std::string& package,
                                     const std::string& workflowName) {
    if (logger) {
        logger->Trace("WorkflowExecuteStep", "LoadWorkflow",
                     "package=" + package + ", workflow=" + workflowName,
                     "Loading");
    }

    // Try to find the workflow in the package.
    std::vector<std::filesystem::path> baseDirs;
    baseDirs.push_back(std::filesystem::current_path() / "gameengine" /
                       "packages");
    baseDirs.push_back(std::filesystem::current_path() / "packages");

    // Also try walking up from current path.
    std::filesystem::path current = std::filesystem::current_path();
    int maxDepth = 5;
    while (current.has_parent_path() && maxDepth-- > 0) {
        std::filesystem::path gameenginePackages =
            current / "gameengine" / "packages";
        if (std::filesystem::exists(gameenginePackages)) {
            baseDirs.push_back(gameenginePackages);
        }
        std::filesystem::path packagesDir = current / "packages";
        if (std::filesystem::exists(packagesDir)) {
            baseDirs.push_back(packagesDir);
        }
        current = current.parent_path();
    }

    for (const auto& baseDir : baseDirs) {
        std::filesystem::path candidate =
            baseDir / package / "workflows" / (workflowName + ".json");
        if (std::filesystem::exists(candidate)) {
            if (logger) {
                logger->Trace("WorkflowExecuteStep", "LoadWorkflow",
                             "Found workflow at: " + candidate.string());
            }

            WorkflowDefinitionParser parser(logger);
            try {
                auto definition = parser.ParseFile(candidate.string());
                if (logger) {
                    logger->Trace(
                        "WorkflowExecuteStep", "LoadWorkflow",
                        "Loaded " + std::to_string(definition.steps.size()) +
                            " steps");
                }
                return definition;
            } catch (const std::exception& e) {
                if (logger) {
                    logger->Error(
                        "WorkflowExecuteStep::LoadWorkflow: Parse error: " +
                        std::string(e.what()));
                }
                continue;
            }
        }
    }

    if (logger) {
        logger->Error(
            "WorkflowExecuteStep::LoadWorkflow: Could not find workflow '" +
            workflowName + "' in package '" + package + "'");
    }

    return WorkflowDefinition();  // Return empty definition on failure.
}

}  // namespace sdl3cpp::services::impl
