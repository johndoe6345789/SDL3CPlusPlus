#include "services/interfaces/workflow/switchback/menu/switchback_menu_settings.hpp"

namespace sdl3cpp::services::impl {

void CloseSwitchbackSettings(SwitchbackSession& session) {
    session.settingsPage = false;
    session.cursor = static_cast<int>(SwitchbackMainRow::Settings);
}

void ChangeSwitchbackSetting(SwitchbackSession& session) {
    switch (static_cast<SwitchbackSettingsRow>(session.cursor)) {
        case SwitchbackSettingsRow::DrawDistance:
            session.drawDistanceStep =
                (session.drawDistanceStep + 1) % kSwitchbackDrawDistanceSteps;
            return;
        case SwitchbackSettingsRow::Dashboard:
            session.showDash = !session.showDash;
            return;
        case SwitchbackSettingsRow::Arrow:
            session.showArrow = !session.showArrow;
            return;
        case SwitchbackSettingsRow::Back:
            CloseSwitchbackSettings(session);
            return;
        case SwitchbackSettingsRow::Count:
            return;
    }
}

}  // namespace sdl3cpp::services::impl
