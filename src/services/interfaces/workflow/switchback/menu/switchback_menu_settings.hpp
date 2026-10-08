#pragma once

#include "services/interfaces/workflow/switchback/session/switchback_session.hpp"

namespace sdl3cpp::services::impl {

/// Applies ENTER on the settings page to the row under the cursor.
void ChangeSwitchbackSetting(SwitchbackSession& session);

/// Leaves the settings page with the cursor on its Settings row.
void CloseSwitchbackSettings(SwitchbackSession& session);

}  // namespace sdl3cpp::services::impl
