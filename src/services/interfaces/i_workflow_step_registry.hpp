#pragma once

#include "services/interfaces/i_workflow_step.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services {

class IWorkflowStepRegistry {
public:
    virtual ~IWorkflowStepRegistry() = default;

    virtual void RegisterStep(std::shared_ptr<IWorkflowStep> step) = 0;
    virtual std::shared_ptr<IWorkflowStep> GetStep(const std::string& pluginId) const = 0;
};

}  // namespace sdl3cpp::services
