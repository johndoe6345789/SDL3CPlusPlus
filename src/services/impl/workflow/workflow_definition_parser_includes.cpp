#include "services/interfaces/workflow/workflow_definition_parser.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <filesystem>
#include <unordered_set>
#include <vector>

namespace sdl3cpp::services::impl {

void WorkflowDefinitionParser::ResolveIncludes(
    std::vector<WorkflowStepDefinition>& steps,
    const std::filesystem::path& baseDir,
    std::unordered_set<std::string>& visited) const {
    std::vector<WorkflowStepDefinition> expanded;
    expanded.reserve(steps.size());

    for (auto& step : steps) {
        if (step.plugin != "workflow.include") {
            expanded.push_back(std::move(step));
            continue;
        }
        ExpandInclude(step, baseDir, visited, expanded);
    }

    steps = std::move(expanded);
}

}  // namespace sdl3cpp::services::impl
