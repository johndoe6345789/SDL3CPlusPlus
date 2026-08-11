#pragma once

#include "services/interfaces/i_command_line_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_platform_service.hpp"
#include <filesystem>
#include <memory>
#include <optional>

namespace sdl3cpp::services::impl {

/**
 * @brief CLI11-based command line parsing service.
 */
class CommandLineService : public ICommandLineService {
public:
    CommandLineService(std::shared_ptr<ILogger> logger,
                       std::shared_ptr<IPlatformService> platformService);

    CommandLineOptions Parse(int argc, char** argv) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IPlatformService> platformService_;

    std::optional<std::filesystem::path> GetDefaultConfigPath() const;
    RuntimeConfig LoadConfigFromJson(const std::filesystem::path& configPath, bool dumpConfig);
    RuntimeConfig LoadDefaultConfig(const char* argv0);
};

}  // namespace sdl3cpp::services::impl
