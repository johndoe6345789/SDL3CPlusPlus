#pragma once

#include "services/interfaces/i_logger.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Small, focused resolver: 1 class = 1 job (resolve workflow paths from packages)
/// Keeps package-aware resolution under 100 LOC
class WorkflowTemplateResolver {
public:
    explicit WorkflowTemplateResolver(std::shared_ptr<ILogger> logger = nullptr);
    /// Resolve workflow by package name and workflow name
    /// Example: ("bootstrap", "boot_default.json") -> "packages/bootstrap/workflows/boot_default.json"
    std::filesystem::path ResolveWorkflow(
        const std::string& packageName,
        const std::string& workflowName) const;

    /// Legacy method for backward compatibility - resolves boot workflow from bootstrap package
    std::filesystem::path ResolveBootTemplate(const std::filesystem::path& configPath) const;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
