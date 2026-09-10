#include "services/interfaces/workflow/workflow_definition_parser.hpp"
#include "services/interfaces/workflow/workflow_definition_parser_includes_internal.hpp"
#include "services/interfaces/workflow_parameter_value.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

void WorkflowDefinitionParser::ExpandInclude(
    WorkflowStepDefinition& step, const std::filesystem::path& baseDir,
    std::unordered_set<std::string>& visited,
    std::vector<WorkflowStepDefinition>& expanded) const {
    // Read the required "path" parameter
    auto pathIt = step.parameters.find("path");
    if (pathIt == step.parameters.end() ||
        pathIt->second.type != WorkflowParameterValue::Type::String ||
        pathIt->second.stringValue.empty()) {
        if (logger_) {
            logger_->Warn("workflow.include step '" + step.id +
                          "' is missing a 'path' parameter — skipping");
        }
        return;
    }

    const std::filesystem::path includePath =
        workflow_include_detail::ResolvePath(pathIt->second.stringValue,
                                             baseDir);

    // Cycle detection
    std::string canonicalKey;
    try {
        canonicalKey = std::filesystem::canonical(includePath).string();
    } catch (...) {
        canonicalKey = includePath.string();
    }
    if (visited.count(canonicalKey)) {
        throw std::runtime_error(
            "workflow.include cycle detected: '" + canonicalKey +
            "' is already on the include stack");
    }

    if (logger_) {
        logger_->Info("workflow.include: expanding '" + step.id + "' → " +
                      includePath.string());
    }

    // Parse the included file (its own ResolveIncludes runs inside
    // ParseFile, so nested includes are handled automatically).
    visited.insert(canonicalKey);
    WorkflowDefinition sub = ParseFile(includePath);
    visited.erase(canonicalKey);

    // Namespace included node IDs to prevent collisions when the same
    // sub-workflow is included more than once.
    // e.g. include id "overlay_group" + sub id "q3_hud" →
    // "overlay_group/q3_hud"
    const std::string ns = step.id + "/";
    for (auto& subStep : sub.steps) {
        subStep.id = ns + subStep.id;
        expanded.push_back(std::move(subStep));
    }

    if (logger_) {
        logger_->Info("workflow.include: '" + step.id + "' expanded to " +
                      std::to_string(sub.steps.size()) + " steps");
    }
}

}  // namespace sdl3cpp::services::impl
