#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

class WorkflowStepParameterResolver {
public:
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
};

}  // namespace sdl3cpp::services::impl
