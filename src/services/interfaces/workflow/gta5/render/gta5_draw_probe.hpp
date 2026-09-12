#pragma once

#include "services/interfaces/workflow/gta5/render/gta5_draw_instances.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// Where the first resident instance lands after the view-projection.
///
/// Diagnostic only: it answers "are the draws off-screen, behind the
/// camera, or on-screen but invisible", which the draw count alone
/// cannot.
std::string ProbeGta5FirstInstance(const Gta5StreamState& state,
                                   const Gta5DrawContext& draw);

}  // namespace sdl3cpp::services::impl
