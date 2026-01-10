#pragma once

#include "services/interfaces/i_config_writer_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief JSON-based configuration writer.
 */
class JsonConfigWriterService : public IConfigWriterService {
public:
    explicit JsonConfigWriterService(std::shared_ptr<ILogger> logger);

    void WriteConfig(const RuntimeConfig& config, const std::filesystem::path& configPath) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
