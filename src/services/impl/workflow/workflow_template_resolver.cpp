#include "workflow_template_resolver.hpp"

#include <system_error>
#include <vector>

namespace sdl3cpp::services::impl {

std::filesystem::path WorkflowTemplateResolver::ResolveBootTemplate(
    const std::filesystem::path& configPath) const {
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
