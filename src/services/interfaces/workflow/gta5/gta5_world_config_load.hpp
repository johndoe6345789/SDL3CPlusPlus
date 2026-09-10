#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_config_types.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Parse config/gta5_world.json. Leaves `world` at its defaults and
/// returns false if the file is missing or malformed.
bool LoadGta5WorldConfig(const std::string& path, Gta5WorldConfig& world,
                         const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
