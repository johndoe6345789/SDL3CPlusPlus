#pragma once

namespace sdl3cpp::services::impl {

/// The screen Switchback is showing. The menu step moves between them.
enum class SwitchbackScreen : int { Menu, Race, Finish };

/// The race picked on the menu. Free roam drives without the route.
enum class SwitchbackRaceMode : int { Career, FreeRoam, QuickRace };

/// Rows of the main menu, in order, and of the settings page.
enum class SwitchbackMainRow : int {
    Career,
    FreeRoam,
    QuickRace,
    Settings,
    Count
};

enum class SwitchbackSettingsRow : int {
    DrawDistance,
    Dashboard,
    Arrow,
    Back,
    Count
};

inline constexpr int kSwitchbackDrawDistanceSteps = 4;

/// Metres the far plane reaches for a draw distance step.
float SwitchbackDrawDistanceM(int step);

/// The label the settings page shows for a draw distance step.
const char* SwitchbackDrawDistanceLabel(int step);

/// Menu choices and settings, shared by the steps that drive, route, frame
/// and draw the game. Lives for the whole run.
struct SwitchbackSession {
    SwitchbackScreen screen = SwitchbackScreen::Menu;
    SwitchbackRaceMode mode = SwitchbackRaceMode::Career;
    /// Set by the menu, read and cleared by the restart step.
    bool restartRequested = false;
    int cursor = 0;
    bool settingsPage = false;
    int drawDistanceStep = 2;
    bool showDash = true;
    bool showArrow = true;

    /// The car is driven and held still otherwise.
    bool Racing() const { return screen == SwitchbackScreen::Race; }
    /// The checkpoint route is running: markers, arrow and progress.
    bool OnRoute() const {
        return screen != SwitchbackScreen::Menu &&
               mode != SwitchbackRaceMode::FreeRoam;
    }
    float DrawDistanceM() const {
        return SwitchbackDrawDistanceM(drawDistanceStep);
    }
};

}  // namespace sdl3cpp::services::impl
