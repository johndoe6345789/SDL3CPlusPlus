#include "services/interfaces/workflow/quake3/q3_menu_navigation.hpp"

namespace sdl3cpp::services::impl {

bool UpdateQ3MenuToggle(WorkflowContext& context, const nlohmann::json& screens,
                        const std::string& defaultScreen) {
    bool open             = context.GetBool("q3.menu_open", false);
    const bool escPressed = context.GetBool("input_key_escape_pressed", false);
    if (escPressed) {
        if (open) {
            // If we're on a sub-screen and it has a back, go back rather
            // than close
            const std::string curScreen =
                context.Get<std::string>("q3.menu_screen", defaultScreen);
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
    return open;
}

}  // namespace sdl3cpp::services::impl
