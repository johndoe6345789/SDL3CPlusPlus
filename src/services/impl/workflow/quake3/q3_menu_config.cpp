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

}  // namespace sdl3cpp::services::impl
