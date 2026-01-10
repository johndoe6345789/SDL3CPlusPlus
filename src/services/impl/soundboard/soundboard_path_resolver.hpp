#pragma once

#include "services/interfaces/i_config_service.hpp"

#include <filesystem>
#include <memory>

namespace sdl3cpp::services::impl {

std::filesystem::path ResolveSoundboardPackageRoot(const std::shared_ptr<IConfigService>& configService);

}  // namespace sdl3cpp::services::impl
