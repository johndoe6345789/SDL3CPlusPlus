#include "services/interfaces/workflow/quake3/workflow_q3_weapon_fire_step.hpp"
#include "services/interfaces/workflow/quake3/q3_missile_types.hpp"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <string>

namespace sdl3cpp::services::impl {

namespace {

// Fire intervals in frames at 60 fps (ioq3 values).
uint32_t FireInterval(const std::string& weapon) {
    if (weapon == "weapon_machinegun")      return 6u;
    if (weapon == "weapon_shotgun")         return 30u;
    if (weapon == "weapon_grenadelauncher") return 30u;
    if (weapon == "weapon_rocketlauncher")  return 20u;
    if (weapon == "weapon_lightning")       return 1u;
    if (weapon == "weapon_railgun")         return 50u;
    if (weapon == "weapon_plasmagun")       return 6u;
    if (weapon == "weapon_bfg")             return 200u;
    return 18u; // gauntlet fallback
}

// Whether the weapon is held-fire (auto) or single-shot.
bool IsAutoFire(const std::string& weapon) {
    return weapon == "weapon_machinegun" ||
           weapon == "weapon_lightning"  ||
           weapon == "weapon_plasmagun";
}

// Instant-hit damage per hit (or per pellet for shotgun).
int InstantHitDamage(const std::string& weapon) {
    if (weapon == "weapon_machinegun") return 7;
    if (weapon == "weapon_shotgun")    return 10;  // ×11 pellets
    if (weapon == "weapon_lightning")  return 8;
    if (weapon == "weapon_railgun")    return 100;
    return 0;
}

// Build a perpendicular tangent from a direction vector.
glm::vec3 Tangent(const glm::vec3& dir) {
    glm::vec3 up = (std::abs(dir.y) < 0.9f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    return glm::normalize(glm::cross(dir, up));
}

// Deterministic-ish jitter for shotgun spread (no <random> header needed).
float JitterComponent(int seed) {
    // Returns a value in [-1, 1] cheaply.
    return static_cast<float>((seed % 200) - 100) / 100.f;
}

// Perform a single Bullet raycast; returns true on hit and fills hitPoint.
bool Raycast(btDiscreteDynamicsWorld* world,
             const btVector3& from, const btVector3& to,
             btVector3& hitPoint) {
    btCollisionWorld::ClosestRayResultCallback cb(from, to);
    world->rayTest(from, to, cb);
    if (cb.hasHit()) {
        hitPoint = cb.m_hitPointWorld;
        return true;
    }
    return false;
}

}  // namespace

WorkflowQ3WeaponFireStep::WorkflowQ3WeaponFireStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3WeaponFireStep::GetPluginId() const {
    return "q3.weapon.fire";
}

void WorkflowQ3WeaponFireStep::Execute(const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    const bool fireHeld    = context.GetBool("input_mouse_left", false);
    const bool firePressed = context.GetBool("input_mouse_left_pressed", false);
    const std::string weapon = context.Get<std::string>("q3.current_weapon", "weapon_machinegun");
    const uint32_t frame   = static_cast<uint32_t>(context.GetDouble("loop.iteration", 0.0));
    uint32_t lastFire      = context.Get<uint32_t>("q3.weapon_last_fire_frame", 0u);
    const uint32_t interval = FireInterval(weapon);

    // Determine if we want to fire this frame.
    const bool wantsFire = firePressed || (fireHeld && IsAutoFire(weapon));
    const bool canFire   = lastFire == 0u || frame >= lastFire + interval;

    if (!wantsFire || !canFire) {
        return;
    }

    // --- Ammo check ---
    auto ammo = context.Get<nlohmann::json>("q3.player_ammo", nlohmann::json::object());
    if (weapon != "weapon_gauntlet" && weapon != "weapon_lightning") {
        int currentAmmo = ammo.value(weapon, 0);
        if (currentAmmo <= 0) {
            return; // dry click
        }
        ammo[weapon] = currentAmmo - 1;
    }

    // --- Update fire-timing context ---
    const uint32_t fireFrame = (frame == 0u) ? 1u : frame;
    context.Set<uint32_t>("q3.weapon_last_fire_frame", fireFrame);
    context.Set<uint32_t>("q3.weapon_flash_until_frame", fireFrame + 4u);
    context.Set<bool>("q3.last_shot_hit", false);
    context.Set("q3.player_ammo", ammo);

    // --- Gather camera info ---
    auto cameraState = context.Get<nlohmann::json>("camera.state", nlohmann::json::object());
    auto* world = context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);

    if (!world || !cameraState.contains("pos") || !cameraState.contains("forward")) {
        // No physics world or camera info — still record the fire event.
        if (logger_) logger_->Info("q3.weapon.fire: fired " + weapon + " (no physics)");
        return;
    }

    const auto& posArr = cameraState["pos"];
    const auto& fwdArr = cameraState["forward"];
    const glm::vec3 origin(posArr[0].get<float>(), posArr[1].get<float>(), posArr[2].get<float>());
    const glm::vec3 forward(fwdArr[0].get<float>(), fwdArr[1].get<float>(), fwdArr[2].get<float>());
    const glm::vec3 fwdN = glm::normalize(forward);

    const btVector3 btFrom(origin.x, origin.y, origin.z);

    int pendingDamage = context.Get<int>("q3.pending_damage", 0);
    bool hitThisFrame = false;

    // =====================================================================
    // Instant-hit weapons
    // =====================================================================

    if (weapon == "weapon_machinegun") {
        const btVector3 btTo = btFrom + btVector3(fwdN.x, fwdN.y, fwdN.z) * 120.f;
        btVector3 hp;
        if (Raycast(world, btFrom, btTo, hp)) {
            pendingDamage += InstantHitDamage(weapon);
            hitThisFrame = true;
        }
    }
    else if (weapon == "weapon_shotgun") {
        // 11 pellets with random spread up to ±0.04 world units in tangent plane.
        const glm::vec3 tan1 = Tangent(fwdN);
        const glm::vec3 tan2 = glm::normalize(glm::cross(fwdN, tan1));
        constexpr float kSpread = 0.04f;
        constexpr int   kPellets = 11;
        for (int p = 0; p < kPellets; ++p) {
            const float jx = JitterComponent(p * 7 + 3) * kSpread;
            const float jy = JitterComponent(p * 13 + 7) * kSpread;
            const glm::vec3 pelletDir = glm::normalize(fwdN + tan1 * jx + tan2 * jy);
            const btVector3 btTo = btFrom + btVector3(pelletDir.x, pelletDir.y, pelletDir.z) * 120.f;
            btVector3 hp;
            if (Raycast(world, btFrom, btTo, hp)) {
                pendingDamage += InstantHitDamage(weapon);
                hitThisFrame = true;
            }
        }
    }
    else if (weapon == "weapon_lightning") {
        // Short range: 8 world units.
        const btVector3 btTo = btFrom + btVector3(fwdN.x, fwdN.y, fwdN.z) * 8.f;
        btVector3 hp;
        if (Raycast(world, btFrom, btTo, hp)) {
            pendingDamage += InstantHitDamage(weapon);
            hitThisFrame = true;
        }
    }
    else if (weapon == "weapon_railgun") {
        // Piercing: step past each hit up to 4 times.
        glm::vec3 rayOrigin = origin;
        constexpr int kMaxPierces = 4;
        for (int pierce = 0; pierce < kMaxPierces; ++pierce) {
            const btVector3 btRayFrom(rayOrigin.x, rayOrigin.y, rayOrigin.z);
            const btVector3 btRayTo = btRayFrom + btVector3(fwdN.x, fwdN.y, fwdN.z) * 120.f;
            btVector3 hp;
            if (!Raycast(world, btRayFrom, btRayTo, hp)) break;
            pendingDamage += InstantHitDamage(weapon);
            hitThisFrame = true;
            // Step slightly past the hit point so next iteration continues.
            rayOrigin = glm::vec3(hp.x(), hp.y(), hp.z()) + fwdN * 0.05f;
        }
    }

    // =====================================================================
    // Projectile weapons — append to q3.missiles
    // =====================================================================
    else {
        auto missiles = context.Get<sdl3cpp::q3::MissileList>("q3.missiles", nullptr);
        if (!missiles) {
            missiles = std::make_shared<std::vector<sdl3cpp::q3::Q3Missile>>();
        }

        sdl3cpp::q3::Q3Missile m;
        m.id         = nextMissileId_++;
        m.origin     = origin;
        m.fromPlayer = true;

        if (weapon == "weapon_rocketlauncher") {
            m.type         = sdl3cpp::q3::MissileType::Rocket;
            m.velocity     = fwdN * 25.f;
            m.damage       = 100.f;
            m.splashDamage = 100.f;
            m.splashRadius = 4.f;
        }
        else if (weapon == "weapon_grenadelauncher") {
            m.type         = sdl3cpp::q3::MissileType::Grenade;
            m.velocity     = fwdN * 18.f;
            m.velocity.y  += 3.f;   // lob arc
            m.damage       = 100.f;
            m.splashDamage = 100.f;
            m.splashRadius = 4.f;
        }
        else if (weapon == "weapon_plasmagun") {
            m.type         = sdl3cpp::q3::MissileType::Plasma;
            m.velocity     = fwdN * 30.f;
            m.damage       = 20.f;
            m.splashDamage = 15.f;
            m.splashRadius = 1.5f;
        }
        else if (weapon == "weapon_bfg") {
            m.type         = sdl3cpp::q3::MissileType::BFG;
            m.velocity     = fwdN * 20.f;
            m.damage       = 100.f;
            m.splashDamage = 200.f;
            m.splashRadius = 6.f;
        }

        missiles->push_back(m);
        context.Set("q3.missiles", missiles);
    }

    // =====================================================================
    // Accumulate hit state
    // =====================================================================
    if (hitThisFrame) {
        context.Set<bool>("q3.last_shot_hit", true);
        context.Set<uint32_t>("q3.hit_marker_until_frame", fireFrame + 10u);
        context.Set<int>("q3.pending_damage", pendingDamage);
    }

    if (logger_) logger_->Info("q3.weapon.fire: fired " + weapon);
}

}  // namespace sdl3cpp::services::impl
