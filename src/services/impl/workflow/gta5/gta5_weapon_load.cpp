#include "services/interfaces/workflow/gta5/gta5_weapon_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

void WorkflowGta5WeaponStep::Load(const WorkflowStepDefinition& step,
                                  WorkflowContext& context) {
    loaded_ = true;
    weapons_ = LoadGta5Weapons(Gta5ResolvePath(
        step, context, "weapons_file", "packages/gta5/data/weapons.json"));
    if (logger_) {
        logger_->Info("gta5.weapon: " + std::to_string(weapons_.size()) +
                      " weapons");
    }
}

bool WorkflowGta5WeaponStep::Edge(const nlohmann::json* keys,
                                  const char* name) {
    const bool down = Gta5KeyDown(keys, name);
    bool& was = held_[name];
    const bool pressed = down && !was;
    was = down;
    return pressed;
}

void WorkflowGta5WeaponStep::Reload(Gta5Inventory& inventory,
                                    const Gta5Weapon& weapon) {
    const int index = inventory.current;
    if (index < 0 || weapon.clip <= 0) return;
    const int room = weapon.clip - inventory.clip[index];
    const int rounds = std::min(room, inventory.reserve[index]);
    if (rounds <= 0) return;
    inventory.clip[index] += rounds;
    inventory.reserve[index] -= rounds;
}

}  // namespace sdl3cpp::services::impl
