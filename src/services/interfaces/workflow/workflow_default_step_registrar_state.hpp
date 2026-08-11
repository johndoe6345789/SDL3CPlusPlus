#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step_registry.hpp"
#include <memory>
#include <unordered_set>
#include <string>

namespace sdl3cpp::services::impl {

class WorkflowDefaultStepRegistrar;

class WorkflowStateStepRegistrar {
public:
    static void RegisterStateSteps(const std::unordered_set<std::string>& plugins,
                                  const std::shared_ptr<IWorkflowStepRegistry>& registry,
                                  const std::shared_ptr<ILogger>& logger);
};

}  // namespace sdl3cpp::services::impl
