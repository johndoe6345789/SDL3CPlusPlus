#pragma once

#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

class WorkflowStepIoResolver {
public:
    explicit WorkflowStepIoResolver(std::shared_ptr<ILogger> logger = nullptr);
    std::string GetRequiredInputKey(const WorkflowStepDefinition& step, const std::string& name) const;
    std::string GetRequiredOutputKey(const WorkflowStepDefinition& step, const std::string& name) const;
    std::string GetOptionalParameterValue(const WorkflowStepDefinition& step, const std::string& name, const std::string& defaultValue) const;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
