#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_effects.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"
#include "services/interfaces/workflow/gta5/gta5_weapon_ray.hpp"
#include "services/interfaces/workflow/gta5/gta5_weapons.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services::impl {

/// A rocket or a grenade on its way: a rocket goes off on contact, a
/// grenade when its fuse runs out.
struct Gta5Projectile {
    glm::vec3 at{0.f};
    glm::vec3 velocity{0.f};
    float fuse{0.f};  // seconds left; a rocket's is its flight time
    float damage{0.f};
    float radius{6.f};
    bool rocket{true};
};

/**
 * Plugin ID: gta5.weapon
 *
 * What the player holds and fires. Q, or the pad's LB, opens the weapon
 * wheel over what they own (gta5.wheel.*); the mouse or the right stick
 * picks, letting go takes it. The left button or RT fires -- held down
 * for anything automatic -- R reloads, and a rocket or a grenade flies
 * as a projectile and goes off (gta5.effects). Rounds strike what the
 * camera looks at, push cars about, and leave their mark. Publishes
 * what the HUD shows: gta5.weapon.name, .clip and .reserve. Not while
 * driving, nor with a menu or the map open. What is carried is kept
 * between sessions, moments after it changes.
 */
class WorkflowGta5WeaponStep final : public IWorkflowStep {
public:
    WorkflowGta5WeaponStep(std::shared_ptr<ILogger> logger,
                           std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    void Load(const WorkflowStepDefinition& step, WorkflowContext& context);
    bool Edge(const nlohmann::json* keys, const char* name);
    void Reload(Gta5Inventory& inventory, const Gta5Weapon& weapon);
    void Wheel(WorkflowContext& context, const nlohmann::json* keys,
               Gta5Inventory& inventory);
    void Fire(WorkflowContext& context, const Gta5Weapon& weapon);
    void Throw(WorkflowContext& context, const Gta5Weapon& weapon);
    void Advance(WorkflowContext& context, float dt);
    void Keep(WorkflowContext& context, const Gta5Inventory& in, float dt);
    void Explode(WorkflowContext& context, const glm::vec3& at, float radius,
                 float damage);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    std::vector<Gta5Weapon> weapons_;
    std::vector<Gta5Projectile> flying_;
    std::unordered_map<std::string, bool> held_;
    std::mt19937 rng_{4242};
    float cooldown_{0.f};
    float keepIn_{0.f};  // seconds until the guns may be written again
    float wheelPick_{0.f};  // where the mouse has swung, in items
    bool wheelOpen_{false};
    bool fireHeld_{false};
    bool loaded_{false};
};

}  // namespace sdl3cpp::services::impl
