#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include "services/interfaces/workflow_context.hpp"

#include <rapidjson/document.h>

namespace sdl3cpp::services {
class ILogger;
class IProbeService;
}

namespace sdl3cpp::services::impl {

struct WorkflowResult {
    WorkflowContext context;
    std::shared_ptr<rapidjson::Document> document;
};

class WorkflowConfigPipeline {
public:
    WorkflowConfigPipeline(std::shared_ptr<ILogger> logger,
                           std::shared_ptr<IProbeService> probeService);

    WorkflowResult Execute(const std::filesystem::path& configPath,
                           std::optional<int>* versionOut) const;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IProbeService> probeService_;
};

}  // namespace sdl3cpp::services::impl
