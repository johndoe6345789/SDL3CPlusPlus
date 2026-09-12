#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_frame_cost.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/// Log what one slow frame spent, step by step. "other" is the rest of
/// the frame -- physics, submitting and presenting -- which the gta5
/// steps do not time themselves.
void ReportGta5Hitch(const std::shared_ptr<ILogger>& logger,
                     const Gta5FrameCost& cost, double frameMs);

}  // namespace sdl3cpp::services::impl
