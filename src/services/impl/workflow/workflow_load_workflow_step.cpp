#include "services/interfaces/workflow/workflow_load_workflow_step.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow/workflow_definition_parser.hpp"
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowLoadWorkflowStep::WorkflowLoadWorkflowStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowLoadWorkflowStep", "Constructor", "Entry");
    }
}

std::string WorkflowLoadWorkflowStep::GetPluginId() const {
    return "workflow.load";
}

void WorkflowLoadWorkflowStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowLoadWorkflowStep", "Execute", "Entry");
    }

    try {
        // Get workflow path from context or parameters
        std::string workflowPath = context.GetString("workflow_path", "");
        if (workflowPath.empty()) {
            logger_->Error("WorkflowLoadWorkflowStep: workflow_path not set in context");
            context.Set("workflow_loaded", false);
            return;
        }

        logger_->Info("Loading workflow: " + workflowPath);

        // Parse workflow
        WorkflowDefinitionParser parser(logger_);
        WorkflowDefinition workflow = parser.ParseFile(workflowPath);

        // Store serialized workflow in context as JSON string
        // (Can't store complex object, so we just log success and expect caller to load it themselves)
        context.Set("workflow_loaded", true);
        context.Set("workflow_name", workflow.templateName);

        logger_->Info("Workflow loaded: " + workflow.templateName + " (" + std::to_string(workflow.steps.size()) + " steps)");

    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowLoadWorkflowStep::Execute: " + std::string(e.what()));
        }
        context.Set("workflow_loaded", false);
    }
}

}  // namespace sdl3cpp::services::impl
