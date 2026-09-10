#include "services/interfaces/workflow/workflow_generic_steps/workflow_package_loader.hpp"
#include "services/interfaces/workflow/workflow_definition_parser.hpp"

#include <filesystem>
#include <stdexcept>
#include <vector>

namespace sdl3cpp::services::impl {

const std::string& RequireStringParam(const WorkflowStepDefinition& step,
                                      const char* name,
                                      const char* errorMsg) {
    auto it = step.parameters.find(name);
    if (it == step.parameters.end()) throw std::runtime_error(errorMsg);
    return it->second.stringValue;
}

WorkflowDefinition LoadPackageWorkflow(const std::shared_ptr<ILogger>& logger,
                                       const std::string& package,
                                       const std::string& workflowName) {
    std::vector<std::filesystem::path> baseDirs;
    baseDirs.push_back(std::filesystem::current_path() / "gameengine" /
                       "packages");
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
        auto candidate =
            baseDir / package / "workflows" / (workflowName + ".json");
        if (std::filesystem::exists(candidate)) {
            WorkflowDefinitionParser parser(logger);
            return parser.ParseFile(candidate.string());
        }
    }

    if (logger) {
        logger->Error("workflow package loader: Could not find workflow '" +
                      workflowName + "' in package '" + package + "'");
    }
    return WorkflowDefinition();
}

}  // namespace sdl3cpp::services::impl
