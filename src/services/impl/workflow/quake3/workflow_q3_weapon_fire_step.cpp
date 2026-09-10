#include "services/interfaces/workflow/quake3/workflow_q3_weapon_fire_step.hpp"
#include "services/interfaces/workflow/quake3/q3_weapon_fire_context.hpp"
#include "services/interfaces/workflow/quake3/q3_weapon_stats.hpp"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

#include <string>

namespace sdl3cpp::services::impl {

WorkflowQ3WeaponFireStep::WorkflowQ3WeaponFireStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3WeaponFireStep::GetPluginId() const {
    return "q3.weapon.fire";
}

void WorkflowQ3WeaponFireStep::Execute(const WorkflowStepDefinition&,
                                       WorkflowContext& context) {
    context.Set<bool>("q3.weapon_fired", false);
    const bool fireHeld    = context.GetBool("input_mouse_left", false);
    const bool firePressed = context.GetBool("input_mouse_left_pressed", false);
    const std::string weapon =
        context.Get<std::string>("q3.current_weapon", "weapon_machinegun");
    const auto frame =
        static_cast<uint32_t>(context.GetDouble("loop.iteration", 0.0));
    const uint32_t lastFire =
        context.Get<uint32_t>("q3.weapon_last_fire_frame", 0u);
    const uint32_t interval = q3::WeaponFireInterval(weapon);

    const bool wantsFire =
        firePressed || (fireHeld && q3::IsWeaponAutoFire(weapon));
    const bool canFire = lastFire == 0u || frame >= lastFire + interval;
    if (!wantsFire || !canFire || !ConsumeWeaponAmmo(context, weapon)) {
        return;
    }

    const uint32_t fireFrame = (frame == 0u) ? 1u : frame;
    context.Set<uint32_t>("q3.weapon_last_fire_frame", fireFrame);
    context.Set<uint32_t>("q3.weapon_flash_until_frame", fireFrame + 4u);
    context.Set<bool>("q3.weapon_fired", true);
    context.Set<bool>("q3.last_shot_hit", false);
    if (weapon == "weapon_machinegun") {
        PublishMachinegunFlashSound(context, fireFrame);
    }

    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    glm::vec3 origin, forward;
    if (!world || !ReadCameraFireVectors(context, origin, forward)) {
        // No physics world or camera info — still record the fire event.
        if (logger_) {
            logger_->Info("q3.weapon.fire: fired " + weapon + " (no physics)");
        }
        return;
    }

    int pendingDamage = context.Get<int>("q3.pending_damage", 0);
    const bool hit    = ResolveWeaponShot(context, weapon, origin, forward,
                                          nextMissileId_++, pendingDamage);
    if (hit) {
        context.Set<bool>("q3.last_shot_hit", true);
        context.Set<uint32_t>("q3.hit_marker_until_frame", fireFrame + 10u);
        context.Set<int>("q3.pending_damage", pendingDamage);
    }

    if (logger_) {
        logger_->Info("q3.weapon.fire: fired " + weapon);
    }
}

}  // namespace sdl3cpp::services::impl
