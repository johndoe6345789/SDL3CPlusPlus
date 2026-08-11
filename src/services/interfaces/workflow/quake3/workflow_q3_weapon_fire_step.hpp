#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.weapon.fire
 *
 * Per-frame weapon firing logic. Enforces per-weapon fire rate, performs
 * instant-hit raycasts (machinegun, shotgun, railgun, lightning) and spawns
 * projectile missiles (rocket, grenade, plasma, BFG).
 *
 * Fire rates (frames at 60 fps, from ioq3):
 *   machinegun 6 | shotgun 30 | grenade 30 | rocket 20
 *   lightning  1 | railgun 50 | plasma   6 | bfg  200
 *
 * Reads:
 *   input_mouse_left (bool), input_mouse_left_pressed (bool)
 *   q3.current_weapon (string)
 *   q3.weapon_last_fire_frame (uint32_t)
 *   q3.player_ammo (nlohmann::json — e.g. {"weapon_machinegun": 100})
 *   camera.state (nlohmann::json — "pos":[x,y,z], "forward":[x,y,z])
 *   physics_world (btDiscreteDynamicsWorld*)
 *   q3.missiles (MissileList — initialised here if null)
 *   loop.iteration (double)
 *
 * Writes:
 *   q3.missiles, q3.pending_damage, q3.last_shot_hit,
 *   q3.hit_marker_until_frame, q3.weapon_last_fire_frame,
 *   q3.weapon_flash_until_frame, q3.player_ammo
 */
class WorkflowQ3WeaponFireStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3WeaponFireStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    uint32_t nextMissileId_{1};
};

}  // namespace sdl3cpp::services::impl
