#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>

#include <string>

namespace sdl3cpp::services::impl {

/// Parses packages/quake3/config/menu.json; returns an empty object if the
/// file is missing or fails to parse.
nlohmann::json LoadQ3MenuConfig();

/// Builds the item list for a screen. A "maps" source string is expanded
/// from `maps` (the q3.maps context list) into one {label, action} entry
/// per map; any other `items` value is used verbatim.
nlohmann::json BuildQ3MenuItems(const nlohmann::json& screen,
                                const nlohmann::json& maps);

/**
 * @brief Applies the Escape-key open/close/back toggle to context.
 *
 * When open and the current screen has a "back" target, Escape navigates
 * back instead of closing. Reads/writes q3.menu_open, q3.menu_screen, and
 * q3.menu_selected_item in `context`.
 *
 * @return The menu's open state after handling the toggle.
 */
bool UpdateQ3MenuToggle(WorkflowContext& context,
                        const nlohmann::json& screens,
                        const std::string& defaultScreen);

/// Outcome of one frame's Enter/Q handling.
struct Q3MenuActionResult {
    bool open = true;
    bool mapSelected = false;
    bool quitPressed = false;
};

/**
 * @brief Handles Up/Down navigation and the Enter/Q actions for one frame.
 *
 * Recognizes the "quit", "close", "back", "screen:<name>", and "map:<id>"
 * item actions; writes q3.menu_screen/q3.menu_selected_item/
 * q3.pending_map to `context` as those actions require.
 */
Q3MenuActionResult HandleQ3MenuInput(WorkflowContext& context,
                                     const nlohmann::json& screens,
                                     const std::string& defaultScreen,
                                     const std::string& screen,
                                     const nlohmann::json& items, bool open,
                                     int& selected, ILogger* logger);

}  // namespace sdl3cpp::services::impl
