#include "services/interfaces/workflow/workflow_template_resolver.hpp"

#include <system_error>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowTemplateResolver::WorkflowTemplateResolver(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowTemplateResolver", "Constructor", "Entry");
    }
}

// Single job: Build package-aware workflow path
// Pattern: packages/{packageName}/workflows/{workflowName}
std::filesystem::path WorkflowTemplateResolver::ResolveWorkflow(
    const std::string& packageName,
    const std::string& workflowName) const {
    if (logger_) {
        logger_->Trace("WorkflowTemplateResolver", "ResolveWorkflow", "Entry");
    }

    // Try from current working directory (typical case)
    std::filesystem::path candidate =
        std::filesystem::current_path() / "packages" / packageName / "workflows" / workflowName;

    std::error_code ec;
    if (std::filesystem::exists(candidate, ec)) {
        return candidate;
    }

    // Try relative to executable (fallback)
    candidate = std::filesystem::path("packages") / packageName / "workflows" / workflowName;
    if (std::filesystem::exists(candidate, ec)) {
        return candidate;
    }

    return {};  // Not found
}

// Legacy boot template resolution - delegates to new package-aware method
std::filesystem::path WorkflowTemplateResolver::ResolveBootTemplate(
    const std::filesystem::path& configPath) const {
    if (logger_) {
        logger_->Trace("WorkflowTemplateResolver", "ResolveBootTemplate", "Entry");
    }

    // Boot workflow lives in bootstrap package
    std::filesystem::path result = ResolveWorkflow("bootstrap", "boot_default.json");

    if (!result.empty()) {
        return result;
    }

    // Fallback: try old hardcoded paths for backward compatibility
    const std::filesystem::path templateRelative = "workflows/templates/boot_default.json";
    std::vector<std::filesystem::path> candidates;

    if (!configPath.empty()) {
        candidates.push_back(configPath.parent_path() / templateRelative);
    }
    candidates.push_back(std::filesystem::current_path() / "config" / templateRelative);

    std::error_code ec;
    for (const auto& candidate : candidates) {
        if (!candidate.empty() && std::filesystem::exists(candidate, ec)) {
            return candidate;
        }
    }

    return {};
}

}  // namespace sdl3cpp::services::impl
