#pragma once

#include "services/interfaces/camera_types.hpp"
#include "services/interfaces/graphics_types.hpp"

namespace sdl3cpp::services::impl {

ViewState BuildViewState(const CameraPose& pose, float aspect);

}  // namespace sdl3cpp::services::impl
