#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_config_types.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Parse config/streaming.json. Leaves `streaming` at its defaults and
/// returns false if the file is missing or malformed.
bool LoadGta5StreamingConfig(const std::string& path,
                             Gta5StreamingConfig& streaming,
                             const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
