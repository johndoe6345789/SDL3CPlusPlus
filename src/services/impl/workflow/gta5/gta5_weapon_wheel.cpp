#include "services/interfaces/workflow/gta5/gta5_weapon_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

void WorkflowGta5WeaponStep::Wheel(WorkflowContext& context,
                                   const nlohmann::json* keys,
                                   Gta5Inventory& inventory) {
    // What there is to choose from: bare hands, then everything owned.
    std::vector<int> owned{-1};
    for (std::size_t i = 0; i < weapons_.size(); ++i) {
        if (inventory.Owns(static_cast<int>(i))) {
            owned.push_back(static_cast<int>(i));
        }
    }
    const bool open = Gta5KeyDown(keys, "Q");
    if (open && !wheelOpen_) {
        // Opens on what is in hand.
        const auto at = std::find(owned.begin(), owned.end(),
                                  inventory.current);
        wheelPick_ = static_cast<float>(
            at == owned.end() ? 0 : std::distance(owned.begin(), at));
    }
    if (open) {
        // The mouse, or the right stick, swings round the wheel; the
        // view holds still meanwhile.
        const float mouse = context.Get<float>("input_mouse_rel_x", 0.f);
        const auto pad = context.Get<glm::vec2>("gta5.pad.look",
                                                glm::vec2(0.f));
        wheelPick_ += mouse * 0.02f + pad.x * 0.25f;
        context.Set<float>("input_mouse_rel_x", 0.f);
        context.Set<float>("input_mouse_rel_y", 0.f);
        const float count = static_cast<float>(owned.size());
        wheelPick_ = std::clamp(wheelPick_, 0.f, count - 1.f);
    } else if (wheelOpen_) {
        inventory.current = owned[static_cast<std::size_t>(wheelPick_ + 0.5f)];
    }
    wheelOpen_ = open;
    // What the HUD draws: the names round the wheel, and which is under
    // the hand.
    std::vector<std::string> names;
    for (const int weapon : owned) {
        names.push_back(weapon < 0 ? "UNARMED" : weapons_[weapon].name);
    }
    context.Set<bool>("gta5.wheel.open", wheelOpen_);
    context.Set<std::vector<std::string>>("gta5.wheel.items", names);
    context.Set<int>("gta5.wheel.selected",
                     static_cast<int>(wheelPick_ + 0.5f));
}

}  // namespace sdl3cpp::services::impl
