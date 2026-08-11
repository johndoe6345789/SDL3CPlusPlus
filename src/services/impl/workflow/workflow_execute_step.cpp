#include "services/interfaces/workflow/workflow_execute_step.hpp"
#include "services/interfaces/workflow_definition.hpp"
#include "services/interfaces/workflow/workflow_definition_parser.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace sdl3cpp::services::impl {

WorkflowExecuteStep::WorkflowExecuteStep(std::shared_ptr<ILogger> logger,
                                         std::shared_ptr<IWorkflowExecutor> executor)
    : logger_(std::move(logger)), executor_(std::move(executor)) {
    if (logger_) {
        logger_->Trace("WorkflowExecuteStep", "Constructor", "Entry");
    }
}

void WorkflowExecuteStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowExecuteStep", "Execute", "Entry");
    }

    // Get parameters
    auto package = step.parameters.find("package");
    auto workflow = step.parameters.find("workflow");

    if (package == step.parameters.end() || workflow == step.parameters.end()) {
        if (logger_) {
            logger_->Warn("WorkflowExecuteStep::Execute: Missing 'package' or 'workflow' parameter");
        }
        return;
    }

    std::string packageName = package->second.stringValue;
    std::string workflowName = workflow->second.stringValue;

    if (packageName.empty() || workflowName.empty()) {
        if (logger_) {
            logger_->Warn("WorkflowExecuteStep::Execute: Empty package or workflow name");
        }
        return;
    }

    if (logger_) {
        logger_->Trace("WorkflowExecuteStep", "Execute",
                       "package=" + packageName + ", workflow=" + workflowName,
                       "Loading child workflow");
    }

    // Load the child workflow using parser
    auto childWorkflow = LoadWorkflow(packageName, workflowName);

    // Execute it with the same context (context passes through)
    executor_->Execute(childWorkflow, context);

    if (logger_) {
        logger_->Trace("WorkflowExecuteStep", "Execute",
                       "workflow=" + workflowName,
                       "Child workflow execution complete");
    }
}

WorkflowDefinition WorkflowExecuteStep::LoadWorkflow(const std::string& package,
                                                     const std::string& workflowName) {
    if (logger_) {
        logger_->Trace("WorkflowExecuteStep", "LoadWorkflow",
                       "package=" + package + ", workflow=" + workflowName,
                       "Loading");
    }

    // Try to find the workflow in the package
    std::vector<std::filesystem::path> baseDirs;
    baseDirs.push_back(std::filesystem::current_path() / "gameengine" / "packages");
    baseDirs.push_back(std::filesystem::current_path() / "packages");

    // Also try walking up from current path
    std::filesystem::path current = std::filesystem::current_path();
    int maxDepth = 5;
    while (current.has_parent_path() && maxDepth-- > 0) {
        std::filesystem::path gameenginePackages = current / "gameengine" / "packages";
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
        std::filesystem::path candidate = baseDir / package / "workflows" / (workflowName + ".json");
        if (std::filesystem::exists(candidate)) {
            if (logger_) {
                logger_->Trace("WorkflowExecuteStep", "LoadWorkflow",
                               "Found workflow at: " + candidate.string());
            }

            // Use WorkflowDefinitionParser to load the workflow
            WorkflowDefinitionParser parser(logger_);
            try {
                auto definition = parser.ParseFile(candidate.string());
                if (logger_) {
                    logger_->Trace("WorkflowExecuteStep", "LoadWorkflow",
                                   "Loaded " + std::to_string(definition.steps.size()) + " steps");
                }
                return definition;
            } catch (const std::exception& e) {
                if (logger_) {
                    logger_->Error("WorkflowExecuteStep::LoadWorkflow: Parse error: " + std::string(e.what()));
                }
                continue;
            }
        }
    }

    if (logger_) {
        logger_->Error("WorkflowExecuteStep::LoadWorkflow: Could not find workflow '" + workflowName +
                       "' in package '" + package + "'");
    }

    return WorkflowDefinition();  // Return empty definition on failure
}

}  // namespace sdl3cpp::services::impl
