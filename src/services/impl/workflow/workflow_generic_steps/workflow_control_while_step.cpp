#include "services/interfaces/workflow/workflow_generic_steps/workflow_control_while_step.hpp"
#include "services/interfaces/workflow/workflow_definition_parser.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <filesystem>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowControlWhileStep::WorkflowControlWhileStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IWorkflowExecutor> executor)
    : logger_(std::move(logger)), executor_(std::move(executor)) {}

std::string WorkflowControlWhileStep::GetPluginId() const {
    return "control.loop.while";
}

void WorkflowControlWhileStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    // Read "condition_key" from parameters - context bool key to check
    auto condIt = step.parameters.find("condition_key");
    if (condIt == step.parameters.end()) {
        throw std::runtime_error("control.loop.while: Missing 'condition_key' parameter");
    }
    std::string conditionKey = condIt->second.stringValue;

    // Read "package" and "workflow" to load the sub-workflow
    auto pkgIt = step.parameters.find("package");
    auto wfIt = step.parameters.find("workflow");
    if (pkgIt == step.parameters.end() || wfIt == step.parameters.end()) {
        throw std::runtime_error("control.loop.while: Missing 'package' or 'workflow' parameter");
    }
    std::string packageName = pkgIt->second.stringValue;
    std::string workflowName = wfIt->second.stringValue;

    // Optional max iterations (safety valve)
    uint32_t maxIterations = 0; // 0 = unlimited
    auto maxIt = step.parameters.find("max_iterations");
    if (maxIt != step.parameters.end() && maxIt->second.type == WorkflowParameterValue::Type::Number) {
        maxIterations = static_cast<uint32_t>(maxIt->second.numberValue);
    }

    // Load the sub-workflow
    auto childWorkflow = LoadWorkflow(packageName, workflowName);
    if (childWorkflow.steps.empty()) {
        throw std::runtime_error("control.loop.while: Could not load workflow '" +
                                 workflowName + "' from package '" + packageName + "'");
    }

    if (logger_) {
        logger_->Info("control.loop.while: Looping on '" + conditionKey +
                      "', workflow=" + workflowName +
                      (maxIterations > 0 ? ", max=" + std::to_string(maxIterations) : ""));
    }

    // Suppress per-step logging inside the frame loop for performance
    context.Set<bool>("_in_frame_loop", true);

    // Execute loop
    uint32_t iteration = 0;
    while (context.GetBool(conditionKey, false)) {
        if (maxIterations > 0 && iteration >= maxIterations) {
            if (logger_) {
                logger_->Warn("control.loop.while: Hit max iterations (" +
                              std::to_string(maxIterations) + ")");
            }
            break;
        }

        context.Set<double>("loop.iteration", static_cast<double>(iteration));
        executor_->Execute(childWorkflow, context);
        iteration++;
    }

    if (logger_) {
        logger_->Info("control.loop.while: Completed after " +
                      std::to_string(iteration) + " iterations");
    }
}

WorkflowDefinition WorkflowControlWhileStep::LoadWorkflow(
    const std::string& package, const std::string& workflowName) {

    std::vector<std::filesystem::path> baseDirs;
    baseDirs.push_back(std::filesystem::current_path() / "gameengine" / "packages");
    baseDirs.push_back(std::filesystem::current_path() / "packages");

    std::filesystem::path current = std::filesystem::current_path();
    int maxDepth = 5;
    while (current.has_parent_path() && maxDepth-- > 0) {
        auto gp = current / "gameengine" / "packages";
        if (std::filesystem::exists(gp)) baseDirs.push_back(gp);
        auto pp = current / "packages";
        if (std::filesystem::exists(pp)) baseDirs.push_back(pp);
        current = current.parent_path();
    }

    for (const auto& baseDir : baseDirs) {
        auto candidate = baseDir / package / "workflows" / (workflowName + ".json");
        if (std::filesystem::exists(candidate)) {
            WorkflowDefinitionParser parser(logger_);
            return parser.ParseFile(candidate.string());
        }
    }

    if (logger_) {
        logger_->Error("control.loop.while: Could not find workflow '" +
                       workflowName + "' in package '" + package + "'");
    }
    return WorkflowDefinition();
}

}  // namespace sdl3cpp::services::impl
