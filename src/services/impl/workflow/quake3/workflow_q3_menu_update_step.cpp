#include "services/interfaces/workflow/quake3/workflow_q3_menu_update_step.hpp"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <string>

namespace sdl3cpp::services::impl {

namespace {

nlohmann::json LoadMenuConfig() {
    std::ifstream f("packages/quake3/config/menu.json");
    if (!f.is_open()) return nlohmann::json{};
    try { return nlohmann::json::parse(f); }
    catch (...) { return nlohmann::json{}; }
}

// Build the item list for a screen. "maps" source is expanded from q3.maps context.
nlohmann::json BuildItems(const nlohmann::json& screen, const nlohmann::json& maps) {
    auto itemsField = screen.find("items");
    if (itemsField == screen.end()) return nlohmann::json::array();

    if (itemsField->is_string() && itemsField->get<std::string>() == "maps") {
        nlohmann::json out = nlohmann::json::array();
        for (const auto& m : maps) {
            std::string name = m.get<std::string>();
            out.push_back({ {"label", name}, {"action", "map:" + name} });
        }
        return out;
    }
    return *itemsField;
}

}  // namespace

WorkflowQ3MenuUpdateStep::WorkflowQ3MenuUpdateStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3MenuUpdateStep::GetPluginId() const {
    return "q3.menu.update";
}

void WorkflowQ3MenuUpdateStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    // Lazy-load menu config once per engine lifetime
    if (!config_loaded_) {
        config_ = LoadMenuConfig();
        config_loaded_ = true;
        if (logger_) logger_->Info("q3.menu.update: loaded menu config");
    }

    const auto& screens = config_.value("screens", nlohmann::json::object());
    const std::string defaultScreen = config_.value("default_screen", std::string("main"));

    // --- toggle open/close ---
    bool open = context.GetBool("q3.menu_open", false);
    const bool escPressed = context.GetBool("input_key_escape_pressed", false);
    if (escPressed) {
        if (open) {
            // If we're on a sub-screen and it has a back, go back rather than close
            const std::string curScreen = context.Get<std::string>("q3.menu_screen", defaultScreen);
            auto screenIt = screens.find(curScreen);
            if (screenIt != screens.end() && screenIt->contains("back")) {
                const std::string back = (*screenIt)["back"].get<std::string>();
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

    // --- build current item list ---
    const std::string screen = context.Get<std::string>("q3.menu_screen", defaultScreen);
    const auto maps = context.Get<nlohmann::json>("q3.maps", nlohmann::json::array({"q3dm7"}));

    nlohmann::json items = nlohmann::json::array();
    std::string title;
    auto screenIt = screens.find(screen);
    if (screenIt != screens.end()) {
        title = screenIt->value("title", screen);
        items = BuildItems(*screenIt, maps);
    }

    context.Set("q3.menu_items", items);
    context.Set<std::string>("q3.menu_title", title);

    // --- navigate ---
    bool mapSelected = false;
    bool quitPressed = false;

    int selected = context.Get<int>("q3.menu_selected_item", 0);
    const int numItems = static_cast<int>(items.size());
    if (numItems > 0) selected = std::clamp(selected, 0, numItems - 1);

    if (open && numItems > 0) {
        if (context.GetBool("input_key_up_pressed", false))
            selected = (selected + numItems - 1) % numItems;
        if (context.GetBool("input_key_down_pressed", false))
            selected = (selected + 1) % numItems;

        if (context.GetBool("input_key_enter_pressed", false)) {
            const std::string action = items[selected].value("action", std::string("none"));
            if (action == "quit") {
                quitPressed = true;
                if (logger_) logger_->Info("q3.menu.update: quit");
            } else if (action == "close") {
                open = false;
                context.Set<bool>("q3.menu_open", open);
            } else if (action == "back") {
                auto sIt = screens.find(screen);
                const std::string back = (sIt != screens.end() && sIt->contains("back"))
                    ? (*sIt)["back"].get<std::string>() : defaultScreen;
                context.Set<std::string>("q3.menu_screen", back);
                context.Set<int>("q3.menu_selected_item", 0);
            } else if (action.rfind("screen:", 0) == 0) {
                context.Set<std::string>("q3.menu_screen", action.substr(7));
                context.Set<int>("q3.menu_selected_item", 0);
            } else if (action.rfind("map:", 0) == 0) {
                const std::string map = action.substr(4);
                context.Set<std::string>("q3.pending_map", map);
                mapSelected = true;
                if (logger_) logger_->Info("q3.menu.update: map selected: " + map);
            }
        }

        if (context.GetBool("input_key_q_pressed", false))
            quitPressed = true;
    }

    context.Set<int>("q3.menu_selected_item", selected);
    context.Set<bool>("q3.menu_map_selected", mapSelected);
    context.Set<bool>("q3.menu_quit_pressed", quitPressed);
}

}  // namespace sdl3cpp::services::impl
