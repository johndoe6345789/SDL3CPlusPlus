#include "../interfaces/i_pipeline_compiler_service.hpp"
#include "../interfaces/i_logger.hpp"
#include <cstdlib>
#include <sstream>
#include <vector>
#include <optional>
#include <string>

namespace sdl3cpp::services::impl {
class PipelineCompilerService : public IPipelineCompilerService {
public:
    PipelineCompilerService(std::shared_ptr<ILogger> logger)
        : logger_(std::move(logger)) {}

    bool Compile(const std::string& inputPath,
                 const std::string& outputPath,
                 const std::vector<std::string>& args = {}) override {
        std::ostringstream cmd;
        cmd << "./src/bgfx_tools/shaderc/shaderc";
        cmd << " -f " << inputPath << " -o " << outputPath;
        for (const auto& arg : args) {
            cmd << " " << arg;
        }
        logger_->Trace("PipelineCompilerService", "Compile", cmd.str());
        int result = std::system(cmd.str().c_str());
        if (result != 0) {
            lastError_ = "bgfx_tools shaderc failed with code " + std::to_string(result);
            logger_->Error(lastError_.value());
            return false;
        }
        lastError_.reset();
        return true;
    }

    std::optional<std::string> GetLastError() const override {
        return lastError_;
    }
private:
    std::shared_ptr<ILogger> logger_;
    std::optional<std::string> lastError_;
};
} // namespace sdl3cpp::services::impl
