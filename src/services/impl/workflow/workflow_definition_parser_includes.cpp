#include "services/interfaces/workflow/workflow_definition_parser.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_parameter_value.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

namespace sdl3cpp::services::impl {

// Resolve the include path:
//   1. Absolute path → use directly.
//   2. Relative path → try CWD first (game assets live under CWD/packages/…),
//      then relative to the including file's directory.
static fs::path ResolvePath(const std::string& rawPath, const fs::path& baseDir) {
    fs::path p(rawPath);
    if (p.is_absolute()) return p;

    // CWD-relative first (matches how shader/asset paths work in workflows)
    const fs::path cwdCandidate = fs::current_path() / p;
    if (fs::exists(cwdCandidate)) return cwdCandidate;

    // Fallback: relative to the including file
    const fs::path relCandidate = baseDir / p;
    if (fs::exists(relCandidate)) return relCandidate;

    // Return CWD-relative even if it doesn't exist yet — ParseFile will throw a
    // better "file not found" message than we would.
    return cwdCandidate;
}

void WorkflowDefinitionParser::ResolveIncludes(
    std::vector<WorkflowStepDefinition>& steps,
    const fs::path& baseDir,
    std::unordered_set<std::string>& visited) const {

    std::vector<WorkflowStepDefinition> expanded;
    expanded.reserve(steps.size());

    for (auto& step : steps) {
        if (step.plugin != "workflow.include") {
            expanded.push_back(std::move(step));
            continue;
        }

        // Read the required "path" parameter
        auto pathIt = step.parameters.find("path");
        if (pathIt == step.parameters.end() ||
            pathIt->second.type != WorkflowParameterValue::Type::String ||
            pathIt->second.stringValue.empty()) {
            if (logger_) {
                logger_->Warn("workflow.include step '" + step.id +
                              "' is missing a 'path' parameter — skipping");
            }
            continue;
        }

        const fs::path includePath = ResolvePath(pathIt->second.stringValue, baseDir);

        // Cycle detection
        std::string canonicalKey;
        try {
            canonicalKey = fs::canonical(includePath).string();
        } catch (...) {
            canonicalKey = includePath.string();
        }
        if (visited.count(canonicalKey)) {
            throw std::runtime_error(
                "workflow.include cycle detected: '" + canonicalKey +
                "' is already on the include stack");
        }

        if (logger_) {
            logger_->Info("workflow.include: expanding '" + step.id +
                          "' → " + includePath.string());
        }

        // Parse the included file (its own ResolveIncludes runs inside ParseFile,
        // so nested includes are handled automatically).
        visited.insert(canonicalKey);
        WorkflowDefinition sub = ParseFile(includePath);
        visited.erase(canonicalKey);

        // Namespace included node IDs to prevent collisions when the same
        // sub-workflow is included more than once.
        // e.g. include id "overlay_group" + sub id "q3_hud" → "overlay_group/q3_hud"
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

    steps = std::move(expanded);
}

}  // namespace sdl3cpp::services::impl
