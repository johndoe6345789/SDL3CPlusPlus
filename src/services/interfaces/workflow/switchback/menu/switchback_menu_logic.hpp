#pragma once

#include "services/interfaces/workflow/switchback/session/switchback_session.hpp"

namespace sdl3cpp::services::impl {

/// The keys the menu reads this frame, as the input drain publishes them.
struct SwitchbackMenuKeys {
    bool up{false};
    bool down{false};
    bool enter{false};
    bool escape{false};
};

/// Moves the session on one frame. A race that has finished goes to the
/// finish screen; Escape leaves any race or finish for the main menu, and
/// backs out of the settings page.
void StepSwitchbackMenu(SwitchbackSession& session,
                        const SwitchbackMenuKeys& keys, bool raceFinished);

}  // namespace sdl3cpp::services::impl
