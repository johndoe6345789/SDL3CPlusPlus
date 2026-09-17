#pragma once

#include "services/interfaces/camera_types.hpp"
#include "services/interfaces/graphics_types.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

ViewState BuildViewState(const CameraPose& pose, float aspect, const std::shared_ptr<ILogger>& logger = nullptr);

}  // namespace sdl3cpp::services::impl
