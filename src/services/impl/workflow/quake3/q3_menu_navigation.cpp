#include "services/interfaces/workflow/quake3/q3_menu_navigation.hpp"

namespace sdl3cpp::services::impl {

Q3MenuActionResult HandleQ3MenuInput(WorkflowContext& context,
                                     const nlohmann::json& screens,
                                     const std::string& defaultScreen,
                                     const std::string& screen,
                                     const nlohmann::json& items, bool open,
                                     int& selected, ILogger* logger) {
    Q3MenuActionResult result;
    result.open = open;

    const int numItems = static_cast<int>(items.size());
    if (open && numItems > 0) {
        if (context.GetBool("input_key_up_pressed", false)) {
            selected = (selected + numItems - 1) % numItems;
        }
        if (context.GetBool("input_key_down_pressed", false)) {
            selected = (selected + 1) % numItems;
        }

        if (context.GetBool("input_key_enter_pressed", false)) {
            const std::string action =
                items[selected].value("action", std::string("none"));
            if (action == "quit") {
                result.quitPressed = true;
                if (logger) logger->Info("q3.menu.update: quit");
            } else if (action == "close") {
                result.open = false;
                context.Set<bool>("q3.menu_open", result.open);
            } else if (action == "back") {
                auto sIt = screens.find(screen);
                const std::string back =
                    (sIt != screens.end() && sIt->contains("back"))
                        ? (*sIt)["back"].get<std::string>()
                        : defaultScreen;
                context.Set<std::string>("q3.menu_screen", back);
                context.Set<int>("q3.menu_selected_item", 0);
            } else if (action.rfind("screen:", 0) == 0) {
                context.Set<std::string>("q3.menu_screen", action.substr(7));
                context.Set<int>("q3.menu_selected_item", 0);
            } else if (action.rfind("map:", 0) == 0) {
                const std::string map = action.substr(4);
                context.Set<std::string>("q3.pending_map", map);
                result.mapSelected = true;
                if (logger) {
                    logger->Info("q3.menu.update: map selected: " + map);
                }
            }
        }

        if (context.GetBool("input_key_q_pressed", false)) {
            result.quitPressed = true;
        }
    }

    return result;
}

}  // namespace sdl3cpp::services::impl
