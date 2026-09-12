#include "services/interfaces/workflow/gta5/hud/gta5_shop_step.hpp"

#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_seat.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

void WorkflowGta5ShopStep::Repaint(WorkflowContext& context,
                                   std::size_t index) {
    if (index >= garage_.colours.size()) return;
    colour_ = static_cast<int>(index);
    for (Gta5Vehicle& car : state_->vehicles) {
        RepaintGta5Vehicle(car, garage_.colours[index].rgb);
    }
    Remember(context);
}

void WorkflowGta5ShopStep::GiveWeapon(WorkflowContext& context,
                                      std::size_t index) {
    if (index >= weapons_.size()) return;
    auto inventory =
        context.Get<Gta5Inventory>("gta5.inventory", Gta5Inventory{});
    inventory.Fit(weapons_.size());
    const Gta5Weapon& weapon = weapons_[index];
    inventory.clip[index] = std::max(weapon.clip, 0);
    inventory.reserve[index] = weapon.max;
    inventory.current = static_cast<int>(index);  // straight into the hand
    context.Set("gta5.inventory", inventory);
    Remember(context);
}

void WorkflowGta5ShopStep::FillAmmo(WorkflowContext& context) {
    auto inventory =
        context.Get<Gta5Inventory>("gta5.inventory", Gta5Inventory{});
    inventory.Fit(weapons_.size());
    for (std::size_t i = 0; i < weapons_.size(); ++i) {
        if (!inventory.Owns(static_cast<int>(i))) continue;
        inventory.clip[i] = std::max(weapons_[i].clip, 0);
        inventory.reserve[i] = weapons_[i].max;
    }
    context.Set("gta5.inventory", inventory);
    Remember(context);
}

void WorkflowGta5ShopStep::Remember(WorkflowContext& context) {
    state_->settings.colour = colour_;
    KeepGta5Settings(
        state_->settings,
        context.Get<float>("gta5.player.health", 100.f),
        context.Get<float>("gta5.player.armour", 0.f),
        context.Get<Gta5Inventory>("gta5.inventory", Gta5Inventory{}));
}

}  // namespace sdl3cpp::services::impl
