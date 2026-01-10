#pragma once

#include "services/interfaces/i_config_compiler_service.hpp"
#include "services/interfaces/i_config_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_probe_service.hpp"
#include "services/interfaces/i_render_graph_service.hpp"
#include "../../../di/lifecycle.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

class ConfigCompilerService : public IConfigCompilerService, public di::IInitializable {
public:
    ConfigCompilerService(std::shared_ptr<IConfigService> configService,
                          std::shared_ptr<IRenderGraphService> renderGraphService,
                          std::shared_ptr<IProbeService> probeService,
                          std::shared_ptr<ILogger> logger);

    void Initialize() override;

    ConfigCompilerResult Compile(const std::string& configJson) override;
    const ConfigCompilerResult& GetLastResult() const override { return lastResult_; }

private:
    void AddDiagnostic(ConfigCompilerResult& result,
                       ProbeSeverity severity,
                       const std::string& code,
                       const std::string& jsonPath,
                       const std::string& message,
                       const std::string& details = "");
    void MergeRenderGraphDiagnostics(ConfigCompilerResult& result,
                                     const RenderGraphBuildResult& renderGraphBuild);

    std::shared_ptr<IConfigService> configService_;
    std::shared_ptr<IRenderGraphService> renderGraphService_;
    std::shared_ptr<IProbeService> probeService_;
    std::shared_ptr<ILogger> logger_;
    ConfigCompilerResult lastResult_{};
};

}  // namespace sdl3cpp::services::impl
