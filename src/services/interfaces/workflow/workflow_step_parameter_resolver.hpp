#pragma once

#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

class WorkflowStepParameterResolver {
public:
    explicit WorkflowStepParameterResolver(std::shared_ptr<ILogger> logger = nullptr);
    const WorkflowParameterValue* FindParameter(const WorkflowStepDefinition& step,
                                                const std::string& name) const;
    const WorkflowParameterValue& GetRequiredParameter(const WorkflowStepDefinition& step,
                                                       const std::string& name) const;
    std::string GetRequiredString(const WorkflowStepDefinition& step, const std::string& name) const;
    double GetRequiredNumber(const WorkflowStepDefinition& step, const std::string& name) const;
    bool GetRequiredBool(const WorkflowStepDefinition& step, const std::string& name) const;
    std::vector<std::string> GetRequiredStringList(const WorkflowStepDefinition& step,
                                                   const std::string& name) const;
    std::vector<double> GetRequiredNumberList(const WorkflowStepDefinition& step,
                                              const std::string& name) const;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
