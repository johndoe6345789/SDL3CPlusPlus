#include "services/interfaces/workflow/quake3/workflow_q3_menu_update_step.hpp"
#include "services/interfaces/workflow/quake3/q3_menu_navigation.hpp"

#include <algorithm>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowQ3MenuUpdateStep::WorkflowQ3MenuUpdateStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3MenuUpdateStep::GetPluginId() const {
    return "q3.menu.update";
}

void WorkflowQ3MenuUpdateStep::Execute(const WorkflowStepDefinition&,
                                       WorkflowContext& context) {
    // Lazy-load menu config once per engine lifetime
    if (!config_loaded_) {
        config_ = LoadQ3MenuConfig();
        config_loaded_ = true;
        if (logger_) logger_->Info("q3.menu.update: loaded menu config");
    }

    const auto& screens = config_.value("screens", nlohmann::json::object());
    const std::string defaultScreen =
        config_.value("default_screen", std::string("main"));

    const bool open = UpdateQ3MenuToggle(context, screens, defaultScreen);

    // --- build current item list ---
    const std::string screen =
        context.Get<std::string>("q3.menu_screen", defaultScreen);
    const auto maps = context.Get<nlohmann::json>(
        "q3.maps", nlohmann::json::array({"q3dm7"}));

    nlohmann::json items = nlohmann::json::array();
    std::string title;
    auto screenIt = screens.find(screen);
    if (screenIt != screens.end()) {
        title = screenIt->value("title", screen);
        items = BuildQ3MenuItems(*screenIt, maps);
    }

    context.Set("q3.menu_items", items);
    context.Set<std::string>("q3.menu_title", title);

    // --- navigate ---
    int selected = context.Get<int>("q3.menu_selected_item", 0);
    const int numItems = static_cast<int>(items.size());
    if (numItems > 0) selected = std::clamp(selected, 0, numItems - 1);

    const Q3MenuActionResult result = HandleQ3MenuInput(
        context, screens, defaultScreen, screen, items, open, selected,
        logger_.get());

    context.Set<int>("q3.menu_selected_item", selected);
    context.Set<bool>("q3.menu_map_selected", result.mapSelected);
    context.Set<bool>("q3.menu_quit_pressed", result.quitPressed);
}

}  // namespace sdl3cpp::services::impl
