#include "workflow_default_step_registrar.hpp"
#include "workflow_config_load_step.hpp"
#include "workflow_config_schema_step.hpp"
#include "workflow_config_version_step.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowDefaultStepRegistrar::WorkflowDefaultStepRegistrar(std::shared_ptr<ILogger> logger,
                                                           std::shared_ptr<IProbeService> probeService)
    : logger_(std::move(logger)),
      probeService_(std::move(probeService)) {}

void WorkflowDefaultStepRegistrar::RegisterDefaults(const std::shared_ptr<IWorkflowStepRegistry>& registry) const {
    if (!registry) {
        throw std::runtime_error("WorkflowDefaultStepRegistrar: registry is null");
    }
    registry->RegisterStep(std::make_shared<WorkflowConfigLoadStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowConfigVersionStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowConfigSchemaStep>(logger_, probeService_));
}

}  // namespace sdl3cpp::services::impl
