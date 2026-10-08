#pragma once

#include "services/interfaces/workflow/gta5/hud/gta5_hud.hpp"
#include "services/interfaces/workflow/switchback/session/switchback_session.hpp"

namespace sdl3cpp::services::impl {

/// The main menu, or the settings page, over the scene while the session is
/// on the menu screen.
Gta5MapFrame BuildSwitchbackMenuFrame(const Gta5Hud& hud, int width,
                                      int height,
                                      const SwitchbackSession& session);

}  // namespace sdl3cpp::services::impl
