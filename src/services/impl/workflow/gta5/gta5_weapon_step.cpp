#include "services/interfaces/workflow/gta5/gta5_weapon_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow/gta5/gta5_weapon_hold.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <algorithm>
#include <utility>

namespace sdl3cpp::services::impl {
WorkflowGta5WeaponStep::WorkflowGta5WeaponStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5WeaponStep::GetPluginId() const {
    return "gta5.weapon";
}

void WorkflowGta5WeaponStep::Execute(const WorkflowStepDefinition& step,
                                     WorkflowContext& context) {
    if (!state_) return;
    if (!loaded_) Load(step, context);
    const float dt =
        std::clamp(context.Get<float>("physics_dt", 1.f / 60.f), 0.f, 0.1f);
    cooldown_ = std::max(0.f, cooldown_ - dt);
    Advance(context, dt);
    auto inventory =
        context.Get<Gta5Inventory>("gta5.inventory", Gta5Inventory{});
    inventory.Fit(weapons_.size());
    const auto* keys = context.TryGet<nlohmann::json>("input.keyboard.state");
    // Not with a shop's menu or the map up, and not from the driver's
    // seat: GTA lets you shoot while driving, this does not yet.
    const bool busy = context.GetBool("gta5.menu.open", false) ||
                      context.GetBool("gta5.map.open", false) ||
                      state_->seated >= 0;
    if (!busy) Wheel(context, keys, inventory);
    const int current = inventory.current;
    const bool armed = current >= 0 &&
                       current < static_cast<int>(weapons_.size()) &&
                       inventory.Owns(current);
    if (armed && !busy && !wheelOpen_) {
        const Gta5Weapon& weapon = weapons_[current];
        const bool down = Gta5TriggerHeld(context);
        const bool pull = weapon.automatic ? down : (down && !fireHeld_);
        fireHeld_ = down;
        if (Edge(keys, "R")) Reload(inventory, weapon);
        if (pull && cooldown_ <= 0.f && inventory.clip[current] != 0) {
            cooldown_ = weapon.rate > 0.f ? 1.f / weapon.rate : 0.5f;
            if (inventory.clip[current] > 0) --inventory.clip[current];
            if (weapon.kind == Gta5WeaponKind::Bullet) {
                Fire(context, weapon);
            } else if (weapon.kind != Gta5WeaponKind::Melee) {
                Throw(context, weapon);
            }
        }
    }
    // What the HUD shows in the corner.
    context.Set<std::string>("gta5.weapon.name",
                             armed ? weapons_[current].name : "");
    context.Set<int>("gta5.weapon.clip", armed ? inventory.clip[current] : -1);
    context.Set<int>("gta5.weapon.reserve",
                     armed ? inventory.reserve[current] : 0);
    context.Set<std::string>("gta5.weapon.model",
                             armed ? weapons_[current].model : "");
    context.Set<int>("gta5.weapon.hands",
                     armed ? weapons_[current].hands : 1);
    context.Set<bool>("gta5.weapon.aiming",
                      armed && !busy && !wheelOpen_ &&
                          Gta5AimHeld(context));
    context.Set("gta5.inventory", inventory);
    Keep(context, inventory, dt);
}

}  // namespace sdl3cpp::services::impl
