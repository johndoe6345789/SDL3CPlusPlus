#include "services/interfaces/workflow/quake3/q3_weapon_projectile.hpp"

namespace sdl3cpp::q3 {

bool BuildWeaponProjectile(const std::string& weapon, const glm::vec3& origin,
                           const glm::vec3& forward, uint32_t id,
                           Q3Missile& out) {
    const glm::vec3 forwardN = glm::normalize(forward);

    Q3Missile m;
    m.id         = id;
    m.origin     = origin;
    m.fromPlayer = true;

    if (weapon == "weapon_rocketlauncher") {
        m.type         = MissileType::Rocket;
        m.velocity     = forwardN * 25.0f;
        m.damage       = 100.0f;
        m.splashDamage = 100.0f;
        m.splashRadius = 4.0f;
    } else if (weapon == "weapon_grenadelauncher") {
        m.type     = MissileType::Grenade;
        m.velocity = forwardN * 18.0f;
        m.velocity.y += 3.0f;  // lob arc
        m.damage       = 100.0f;
        m.splashDamage = 100.0f;
        m.splashRadius = 4.0f;
    } else if (weapon == "weapon_plasmagun") {
        m.type         = MissileType::Plasma;
        m.velocity     = forwardN * 30.0f;
        m.damage       = 20.0f;
        m.splashDamage = 15.0f;
        m.splashRadius = 1.5f;
    } else if (weapon == "weapon_bfg") {
        m.type         = MissileType::BFG;
        m.velocity     = forwardN * 20.0f;
        m.damage       = 100.0f;
        m.splashDamage = 200.0f;
        m.splashRadius = 6.0f;
    } else {
        return false;
    }

    out = m;
    return true;
}

}  // namespace sdl3cpp::q3
