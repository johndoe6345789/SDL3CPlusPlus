#pragma once

#include "services/interfaces/config_types.hpp"
#include "services/interfaces/i_logger.hpp"

#include <filesystem>
#include <memory>

#include <rapidjson/document.h>

namespace sdl3cpp::services::impl {

class RuntimeConfigBuilder final {
public:
    explicit RuntimeConfigBuilder(std::shared_ptr<ILogger> logger);

    services::RuntimeConfig Build(const rapidjson::Document& document,
                                  const std::filesystem::path& configPath) const;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
