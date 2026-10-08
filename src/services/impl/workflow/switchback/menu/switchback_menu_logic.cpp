#include "services/interfaces/workflow/switchback/menu/switchback_menu_logic.hpp"

#include "services/interfaces/workflow/switchback/menu/switchback_menu_settings.hpp"

namespace sdl3cpp::services::impl {
namespace {

void ToMainMenu(SwitchbackSession& session) {
    session.screen = SwitchbackScreen::Menu;
    CloseSwitchbackSettings(session);
}

void StartRace(SwitchbackSession& session, SwitchbackRaceMode mode) {
    session.mode = mode;
    session.screen = SwitchbackScreen::Race;
    session.restartRequested = true;
}

void OpenMainRow(SwitchbackSession& session) {
    switch (static_cast<SwitchbackMainRow>(session.cursor)) {
        case SwitchbackMainRow::Career:
            StartRace(session, SwitchbackRaceMode::Career);
            return;
        case SwitchbackMainRow::FreeRoam:
            StartRace(session, SwitchbackRaceMode::FreeRoam);
            return;
        case SwitchbackMainRow::QuickRace:
            StartRace(session, SwitchbackRaceMode::QuickRace);
            return;
        case SwitchbackMainRow::Settings:
            session.settingsPage = true;
            session.cursor = 0;
            return;
        case SwitchbackMainRow::Count:
            return;
    }
}

int RowCount(const SwitchbackSession& session) {
    return session.settingsPage
               ? static_cast<int>(SwitchbackSettingsRow::Count)
               : static_cast<int>(SwitchbackMainRow::Count);
}

void MoveCursor(SwitchbackSession& session, const SwitchbackMenuKeys& keys) {
    const int rows = RowCount(session);
    if (keys.up) session.cursor = (session.cursor + rows - 1) % rows;
    if (keys.down) session.cursor = (session.cursor + 1) % rows;
}

}  // namespace

void StepSwitchbackMenu(SwitchbackSession& session,
                        const SwitchbackMenuKeys& keys, bool raceFinished) {
    if (session.screen == SwitchbackScreen::Race && raceFinished &&
        session.OnRoute()) {
        session.screen = SwitchbackScreen::Finish;
    }
    if (session.screen != SwitchbackScreen::Menu) {
        if (keys.escape) ToMainMenu(session);
        return;
    }
    if (keys.escape) {
        if (session.settingsPage) CloseSwitchbackSettings(session);
        return;
    }
    MoveCursor(session, keys);
    if (!keys.enter) return;
    if (session.settingsPage) {
        ChangeSwitchbackSetting(session);
    } else {
        OpenMainRow(session);
    }
}

}  // namespace sdl3cpp::services::impl
