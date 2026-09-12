#include "services/interfaces/workflow/gta5/weapon/gta5_weapon_step.hpp"

#include "services/interfaces/workflow/gta5/hud/gta5_settings.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

void WorkflowGta5WeaponStep::Keep(WorkflowContext& context,
                                  const Gta5Inventory& inventory, float dt) {
    keepIn_ = std::max(0.f, keepIn_ - dt);
    // A run that loaded no weapons carries nothing worth keeping.
    if (weapons_.empty() || inventory.clip.empty()) return;
    const Gta5Inventory& kept = state_->settings.inventory;
    if (kept.current == inventory.current && kept.clip == inventory.clip &&
        kept.reserve == inventory.reserve) {
        return;  // nothing has been taken, fired or reloaded
    }
    if (keepIn_ > 0.f) return;  // writing the file a round would be daft
    keepIn_ = 3.f;
    KeepGta5Settings(state_->settings,
                     context.Get<float>("gta5.player.health", 100.f),
                     context.Get<float>("gta5.player.armour", 0.f),
                     inventory);
}

}  // namespace sdl3cpp::services::impl
