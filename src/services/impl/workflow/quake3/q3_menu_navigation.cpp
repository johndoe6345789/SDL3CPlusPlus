#include "services/interfaces/workflow/quake3/q3_menu_navigation.hpp"

#include <fstream>

namespace sdl3cpp::services::impl {

nlohmann::json LoadQ3MenuConfig() {
    std::ifstream f("packages/quake3/config/menu.json");
    if (!f.is_open()) return nlohmann::json{};
    try {
        return nlohmann::json::parse(f);
    } catch (...) {
        return nlohmann::json{};
    }
}

nlohmann::json BuildQ3MenuItems(const nlohmann::json& screen,
                                const nlohmann::json& maps) {
    auto itemsField = screen.find("items");
    if (itemsField == screen.end()) return nlohmann::json::array();

    if (itemsField->is_string() && itemsField->get<std::string>() == "maps") {
        nlohmann::json out = nlohmann::json::array();
        for (const auto& m : maps) {
            std::string name = m.get<std::string>();
            out.push_back({{"label", name}, {"action", "map:" + name}});
        }
        return out;
    }
    return *itemsField;
}

bool UpdateQ3MenuToggle(WorkflowContext& context,
                        const nlohmann::json& screens,
                        const std::string& defaultScreen) {
    bool open = context.GetBool("q3.menu_open", false);
    const bool escPressed =
        context.GetBool("input_key_escape_pressed", false);
    if (escPressed) {
        if (open) {
            // If we're on a sub-screen and it has a back, go back rather
            // than close
            const std::string curScreen =
                context.Get<std::string>("q3.menu_screen", defaultScreen);
            auto screenIt = screens.find(curScreen);
            if (screenIt != screens.end() && screenIt->contains("back")) {
                const std::string back =
                    (*screenIt)["back"].get<std::string>();
                context.Set<std::string>("q3.menu_screen", back);
                context.Set<int>("q3.menu_selected_item", 0);
            } else {
                open = false;
            }
        } else {
            open = true;
            context.Set<std::string>("q3.menu_screen", defaultScreen);
            context.Set<int>("q3.menu_selected_item", 0);
        }
    }
    context.Set<bool>("q3.menu_open", open);
    return open;
}

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
                context.Set<std::string>("q3.menu_screen",
                                         action.substr(7));
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
