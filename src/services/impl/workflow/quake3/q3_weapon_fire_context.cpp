#include "services/interfaces/workflow/quake3/q3_weapon_fire_context.hpp"
#include "services/interfaces/workflow/quake3/q3_missile_types.hpp"
#include "services/interfaces/workflow/quake3/q3_weapon_hitscan.hpp"
#include "services/interfaces/workflow/quake3/q3_weapon_projectile.hpp"
#include "services/interfaces/workflow/quake3/q3_weapon_stats.hpp"

#include <btBulletDynamicsCommon.h>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

void PublishMachinegunFlashSound(WorkflowContext& context, uint32_t fireFrame) {
    static const char* kFlash[] = {"sound/weapons/machinegun/machgf1b.wav",
                                   "sound/weapons/machinegun/machgf2b.wav",
                                   "sound/weapons/machinegun/machgf3b.wav",
                                   "sound/weapons/machinegun/machgf4b.wav"};
    context.Set<std::string>("q3.weapon_fire_sound", kFlash[fireFrame & 3u]);
}

bool ConsumeWeaponAmmo(WorkflowContext& context, const std::string& weapon) {
    auto ammo =
        context.Get<nlohmann::json>("q3.player_ammo", nlohmann::json::object());
    if (weapon != "weapon_gauntlet" && weapon != "weapon_lightning") {
        const int currentAmmo = ammo.value(weapon, 0);
        if (currentAmmo <= 0) {
            return false;  // dry click
        }
        ammo[weapon] = currentAmmo - 1;
    }
    context.Set("q3.player_ammo", ammo);
    return true;
}

bool ReadCameraFireVectors(WorkflowContext& context, glm::vec3& origin,
                           glm::vec3& forward) {
    const auto cameraState =
        context.Get<nlohmann::json>("camera.state", nlohmann::json::object());
    if (!cameraState.contains("pos") || !cameraState.contains("forward")) {
        return false;
    }
    const auto& posArr = cameraState["pos"];
    const auto& fwdArr = cameraState["forward"];
    origin             = {posArr[0].get<float>(), posArr[1].get<float>(),
                          posArr[2].get<float>()};
    forward            = {fwdArr[0].get<float>(), fwdArr[1].get<float>(),
                          fwdArr[2].get<float>()};
    return true;
}

bool ResolveWeaponShot(WorkflowContext& context, const std::string& weapon,
                       const glm::vec3& origin, const glm::vec3& forward,
                       uint32_t nextMissileId, int& pendingDamage) {
    if (q3::WeaponInstantHitDamage(weapon) > 0) {
        auto* world =
            context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
        const auto result =
            q3::FireHitscanWeapon(world, weapon, origin, forward);
        pendingDamage += result.damage;
        return result.hit;
    }

    auto missiles = context.Get<q3::MissileList>("q3.missiles", nullptr);
    if (!missiles) {
        missiles = std::make_shared<std::vector<q3::Q3Missile>>();
    }
    q3::Q3Missile m;
    m.id         = nextMissileId;
    m.origin     = origin;
    m.fromPlayer = true;
    q3::BuildWeaponProjectile(weapon, origin, forward, nextMissileId, m);
    missiles->push_back(m);
    context.Set("q3.missiles", missiles);
    return false;
}

}  // namespace sdl3cpp::services::impl
