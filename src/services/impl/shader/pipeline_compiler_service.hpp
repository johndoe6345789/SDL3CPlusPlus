#pragma once

#include "services/interfaces/i_pipeline_compiler_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

class PipelineCompilerService : public IPipelineCompilerService {
public:
    PipelineCompilerService(std::shared_ptr<sdl3cpp::services::ILogger> logger);
    ~PipelineCompilerService() override = default;

    bool Compile(const std::string& inputPath,
                 const std::string& outputPath,
                 const std::vector<std::string>& args = {}) override;

    std::optional<std::string> GetLastError() const override;

private:
    std::shared_ptr<sdl3cpp::services::ILogger> logger_;
    std::optional<std::string> lastError_;
};

} // namespace sdl3cpp::services::impl