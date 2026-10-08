#include "services/interfaces/workflow/switchback/dash/switchback_menu_frame.hpp"

#include "services/interfaces/workflow/gta5/hud/gta5_hud_parts.hpp"
#include "services/interfaces/workflow/gta5/hud/gta5_hud_state.hpp"
#include "services/interfaces/workflow/gta5/hud/gta5_map_frame.hpp"
#include "services/interfaces/workflow/gta5/hud/gta5_menu.hpp"

#include <string>

namespace sdl3cpp::services::impl {
namespace {

std::string OnOff(bool on) { return on ? "ON" : "OFF"; }

Gta5Menu MainMenu(const SwitchbackSession& session) {
    Gta5Menu menu;
    menu.title = "SWITCHBACK";
    menu.items = {"CAREER", "FREE ROAM", "QUICK RACE", "SETTINGS"};
    menu.selected = session.cursor;
    menu.hint = "ENTER OPEN";
    return menu;
}

Gta5Menu SettingsMenu(const SwitchbackSession& session) {
    const char* distance =
        SwitchbackDrawDistanceLabel(session.drawDistanceStep);
    Gta5Menu menu;
    menu.title = "SETTINGS";
    menu.items = {
        std::string("DRAW DISTANCE  ") + distance,
        "DASHBOARD  " + OnOff(session.showDash),
        "ARROW  " + OnOff(session.showArrow),
        "BACK",
    };
    menu.selected = session.cursor;
    menu.hint = "ENTER CHANGE  ESC BACK";
    return menu;
}

}  // namespace

Gta5MapFrame BuildSwitchbackMenuFrame(const Gta5Hud& hud, int width,
                                      int height,
                                      const SwitchbackSession& session) {
    Gta5MapFrame frame;
    Gta5HudState state;
    state.menuOpen = true;
    state.menu = session.settingsPage ? SettingsMenu(session)
                                      : MainMenu(session);
    AddGta5HudMenu(frame, FitGta5Map(width, height), hud, state,
                   float(width), float(height));
    return frame;
}

}  // namespace sdl3cpp::services::impl
