#pragma once

#include "services/interfaces/i_render_graph_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_probe_service.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

class RenderGraphService : public IRenderGraphService {
public:
    RenderGraphService(std::shared_ptr<ILogger> logger,
                       std::shared_ptr<IProbeService> probeService = nullptr);

    RenderGraphBuildResult BuildGraph(const RenderGraphIR& renderGraph) override;

private:
    void AddDiagnostic(RenderGraphBuildResult& result,
                       ProbeSeverity severity,
                       const std::string& code,
                       const std::string& jsonPath,
                       const std::string& message,
                       const std::string& details = "") const;

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IProbeService> probeService_;
};

}  // namespace sdl3cpp::services::impl
