#include "services/interfaces/workflow/quake3/q3_pickup_effects.hpp"

#include <algorithm>
#include <sstream>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kTouchRadius = 1.0f;

// Respawn times (seconds) matching ioq3 defaults.
constexpr double kRespawnArmor  = 25.0;
constexpr double kRespawnHealth = 35.0;
constexpr double kRespawnAmmo   = 40.0;
constexpr double kRespawnWeapon = 30.0;

bool HasPrefix(const std::string& s, const std::string& prefix) {
    return s.rfind(prefix, 0) == 0;
}

// Parse "x y z" origin string into glm::vec3; returns false on failure.
bool ParseOrigin(const std::string& origin, glm::vec3& out) {
    std::istringstream ss(origin);
    float x = 0.0f, y = 0.0f, z = 0.0f;
    if (!(ss >> x >> y >> z)) return false;
    out = glm::vec3(x, y, z);
    return true;
}

// ioq3 default ammo per ammo_* classname.
int DefaultAmmoAmount(const std::string& cls) {
    if (cls == "ammo_bullets")   return 50;
    if (cls == "ammo_shells")    return 10;
    if (cls == "ammo_grenades")  return  5;
    if (cls == "ammo_rockets")   return  5;
    if (cls == "ammo_cells")     return 30;
    if (cls == "ammo_lightning") return 60;
    if (cls == "ammo_slugs")     return 10;
    if (cls == "ammo_bfg")       return 15;
    return 10;
}

// Map ammo_* classname to the weapon key used in q3.player_ammo.
std::string AmmoWeaponKey(const std::string& cls) {
    if (cls == "ammo_bullets")   return "weapon_machinegun";
    if (cls == "ammo_shells")    return "weapon_shotgun";
    if (cls == "ammo_grenades")  return "weapon_grenadelauncher";
    if (cls == "ammo_rockets")   return "weapon_rocketlauncher";
    if (cls == "ammo_cells")     return "weapon_plasmagun";
    if (cls == "ammo_lightning") return "weapon_lightning";
    if (cls == "ammo_slugs")     return "weapon_railgun";
    if (cls == "ammo_bfg")       return "weapon_bfg";
    return cls;  // fallback: use classname as key
}

// Default ammo granted when picking up a weapon_* entity.
int DefaultWeaponAmmo(const std::string& cls) {
    if (cls == "weapon_machinegun")      return 100;
    if (cls == "weapon_shotgun")         return  10;
    if (cls == "weapon_grenadelauncher") return   5;
    if (cls == "weapon_rocketlauncher")  return   5;
    if (cls == "weapon_lightning")       return  60;
    if (cls == "weapon_railgun")         return  10;
    if (cls == "weapon_plasmagun")       return  50;
    if (cls == "weapon_bfg")             return  15;
    return 10;
}

// Reads the entity's world position from "origin" ("x y z" string) or
// "position" ([x, y, z] array). Returns false if neither is present.
bool ReadEntityPosition(const nlohmann::json& ent, glm::vec3& out) {
    if (ent.contains("origin") && ent["origin"].is_string()) {
        return ParseOrigin(ent["origin"].get<std::string>(), out);
    }
    if (ent.contains("position") && ent["position"].is_array()) {
        const auto& p = ent["position"];
        if (p.size() == 3) {
            out = glm::vec3(p[0].get<float>(), p[1].get<float>(),
                            p[2].get<float>());
            return true;
        }
    }
    return false;
}

// Applies one pickup's effect to `state`, returning its respawn delay.
double ApplyOnePickup(const std::string& cls, PickupTouchState& state) {
    if (cls.find("item_health") != std::string::npos) {
        if (cls == "item_health") {
            state.health = std::min(state.health + 25, 100);
        } else if (cls == "item_health_large") {
            state.health = std::min(state.health + 50, 100);
        } else if (cls == "item_health_mega") {
            state.health = std::min(state.health + 100, 200);
        } else {
            state.health = std::min(state.health + 25, 100);
        }
        return kRespawnHealth;
    }
    if (cls.find("item_armor") != std::string::npos) {
        if (cls == "item_armor_shard") {
            state.armor = std::min(state.armor + 5, 200);
        } else if (cls == "item_armor_combat") {
            state.armor = std::min(state.armor + 50, 200);
            state.armorType = "green";
        } else if (cls == "item_armor_body") {
            state.armor = std::min(state.armor + 100, 200);
            state.armorType = "yellow";
        }
        return kRespawnArmor;
    }
    if (HasPrefix(cls, "ammo_")) {
        const std::string weaponKey = AmmoWeaponKey(cls);
        const int current = state.ammo.value(weaponKey, 0);
        state.ammo[weaponKey] = current + DefaultAmmoAmount(cls);
        return kRespawnAmmo;
    }
    if (HasPrefix(cls, "weapon_")) {
        state.inventory[cls] = true;
        const int current = state.ammo.value(cls, 0);
        state.ammo[cls] = current + DefaultWeaponAmmo(cls);
        return kRespawnWeapon;
    }
    return kRespawnHealth;
}

}  // namespace

void ApplyPickupTouches(const nlohmann::json& entities,
                        const glm::vec3& playerPos, double elapsed,
                        PickupTouchState& state,
                        const std::shared_ptr<ILogger>& logger) {
    for (const auto& ent : entities) {
        const std::string cls = ent.value("classname", std::string{});
        if (cls.empty()) continue;

        const bool isHealth = cls.find("item_health") != std::string::npos;
        const bool isArmor  = cls.find("item_armor") != std::string::npos;
        const bool isAmmo   = HasPrefix(cls, "ammo_");
        const bool isWeapon = HasPrefix(cls, "weapon_");
        if (!isHealth && !isArmor && !isAmmo && !isWeapon) continue;

        const std::string id = ent.value("id", std::string{});
        if (id.empty()) continue;
        if (state.collected.value(id, false)) continue;

        glm::vec3 entPos(0.0f);
        if (!ReadEntityPosition(ent, entPos)) continue;
        if (glm::distance(playerPos, entPos) >= kTouchRadius) continue;

        const double respawnDelay = ApplyOnePickup(cls, state);
        state.collected[id]    = true;
        state.respawnTimes[id] = elapsed + respawnDelay;

        if (logger) {
            logger->Info("q3.pickups.touch: collected " + cls + " (id=" +
                        id + ")");
        }
    }
}

}  // namespace sdl3cpp::services::impl
