#include "workflow_default_step_registrar.hpp"
#include "workflow_config_load_step.hpp"
#include "workflow_config_migration_step.hpp"
#include "workflow_config_schema_step.hpp"
#include "workflow_config_version_step.hpp"

#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowDefaultStepRegistrar::WorkflowDefaultStepRegistrar(std::shared_ptr<ILogger> logger,
                                                           std::shared_ptr<IProbeService> probeService)
    : logger_(std::move(logger)),
      probeService_(std::move(probeService)) {}

void WorkflowDefaultStepRegistrar::RegisterUsedSteps(
    const WorkflowDefinition& workflow,
    const std::shared_ptr<IWorkflowStepRegistry>& registry) const {
    if (!registry) {
        throw std::runtime_error("WorkflowDefaultStepRegistrar: registry is null");
    }

    std::unordered_set<std::string> plugins;
    for (const auto& step : workflow.steps) {
        plugins.insert(step.plugin);
    }

    if (plugins.contains("config.load")) {
        registry->RegisterStep(std::make_shared<WorkflowConfigLoadStep>(logger_));
    }
    if (plugins.contains("config.version.validate")) {
        registry->RegisterStep(std::make_shared<WorkflowConfigVersionStep>(logger_));
    }
    if (plugins.contains("config.migrate")) {
        registry->RegisterStep(std::make_shared<WorkflowConfigMigrationStep>(logger_, probeService_));
    }
    if (plugins.contains("config.schema.validate")) {
        registry->RegisterStep(std::make_shared<WorkflowConfigSchemaStep>(logger_, probeService_));
    }
}

}  // namespace sdl3cpp::services::impl
