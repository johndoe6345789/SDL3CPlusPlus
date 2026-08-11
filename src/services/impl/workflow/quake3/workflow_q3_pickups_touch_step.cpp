#include "services/interfaces/workflow/quake3/workflow_q3_pickups_touch_step.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <sstream>
#include <string>

namespace sdl3cpp::services::impl {

namespace {

constexpr float kTouchRadius = 1.0f;

// Respawn times (seconds) matching ioq3 defaults
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

// ioq3 default ammo per ammo_* classname
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

// Map ammo_* classname to the weapon key used in q3.player_ammo
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

// Default ammo granted when picking up a weapon_* entity
int DefaultWeaponAmmo(const std::string& cls) {
    if (cls == "weapon_machinegun")     return 100;
    if (cls == "weapon_shotgun")        return  10;
    if (cls == "weapon_grenadelauncher")return   5;
    if (cls == "weapon_rocketlauncher") return   5;
    if (cls == "weapon_lightning")      return  60;
    if (cls == "weapon_railgun")        return  10;
    if (cls == "weapon_plasmagun")      return  50;
    if (cls == "weapon_bfg")            return  15;
    return 10;
}

}  // namespace

WorkflowQ3PickupsTouchStep::WorkflowQ3PickupsTouchStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3PickupsTouchStep::GetPluginId() const { return "q3.pickups.touch"; }

void WorkflowQ3PickupsTouchStep::Execute(const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    if (context.GetBool("q3.player_dead", false)) return;

    const auto* entitiesPtr = context.TryGet<nlohmann::json>("bsp.entities");
    if (!entitiesPtr || !entitiesPtr->is_array()) return;

    const glm::vec3 playerPos = context.Get<glm::vec3>("q3.player_pos", glm::vec3(0.0f));
    int health    = context.GetInt("q3.player_health", 100);
    int armor     = context.GetInt("q3.player_armor",  0);
    std::string armorType = context.GetString("q3.armor_type", "none");
    const double elapsed  = context.GetDouble("frame.elapsed", 0.0);

    auto ammo      = context.Get<nlohmann::json>("q3.player_ammo",          nlohmann::json::object());
    auto inventory = context.Get<nlohmann::json>("q3.inventory",            nlohmann::json::object());
    auto collected = context.Get<nlohmann::json>("q3.collected",            nlohmann::json::object());
    auto respawnTimes = context.Get<nlohmann::json>("q3.pickup_respawn_times", nlohmann::json::object());

    for (const auto& ent : *entitiesPtr) {
        const std::string cls = ent.value("classname", std::string{});
        if (cls.empty()) continue;

        // Only consider pickups
        const bool isHealth = cls.find("item_health") != std::string::npos;
        const bool isArmor  = cls.find("item_armor")  != std::string::npos;
        const bool isAmmo   = HasPrefix(cls, "ammo_");
        const bool isWeapon = HasPrefix(cls, "weapon_");
        if (!isHealth && !isArmor && !isAmmo && !isWeapon) continue;

        // Entity ID for collection tracking
        const std::string id = ent.value("id", std::string{});
        if (id.empty()) continue;
        if (collected.value(id, false)) continue;

        // Parse entity position from "origin" field ("x y z")
        glm::vec3 entPos(0.0f);
        if (ent.contains("origin") && ent["origin"].is_string()) {
            if (!ParseOrigin(ent["origin"].get<std::string>(), entPos)) continue;
        } else if (ent.contains("position") && ent["position"].is_array()) {
            const auto& p = ent["position"];
            if (p.size() == 3) {
                entPos = glm::vec3(p[0].get<float>(), p[1].get<float>(), p[2].get<float>());
            }
        } else {
            continue;
        }

        if (glm::distance(playerPos, entPos) >= kTouchRadius) continue;

        // Apply pickup effect
        double respawnDelay = kRespawnHealth;

        if (isHealth) {
            respawnDelay = kRespawnHealth;
            if (cls == "item_health")        health = std::min(health + 25,  100);
            else if (cls == "item_health_large") health = std::min(health + 50,  100);
            else if (cls == "item_health_mega")  health = std::min(health + 100, 200);
            else                                 health = std::min(health + 25,  100);
        } else if (isArmor) {
            respawnDelay = kRespawnArmor;
            if (cls == "item_armor_shard") {
                armor = std::min(armor + 5, 200);
            } else if (cls == "item_armor_combat") {
                armor = std::min(armor + 50, 200);
                armorType = "green";
            } else if (cls == "item_armor_body") {
                armor = std::min(armor + 100, 200);
                armorType = "yellow";
            }
        } else if (isAmmo) {
            respawnDelay = kRespawnAmmo;
            const std::string weaponKey = AmmoWeaponKey(cls);
            const int current = ammo.value(weaponKey, 0);
            ammo[weaponKey]   = current + DefaultAmmoAmount(cls);
        } else if (isWeapon) {
            respawnDelay = kRespawnWeapon;
            inventory[cls] = true;
            const int current = ammo.value(cls, 0);
            ammo[cls] = current + DefaultWeaponAmmo(cls);
        }

        collected[id]     = true;
        respawnTimes[id]  = elapsed + respawnDelay;

        if (logger_) logger_->Info("q3.pickups.touch: collected " + cls + " (id=" + id + ")");
    }

    context.Set("q3.player_health",        health);
    context.Set("q3.player_armor",         armor);
    context.Set("q3.armor_type",           armorType);
    context.Set("q3.player_ammo",          ammo);
    context.Set("q3.inventory",            inventory);
    context.Set("q3.collected",            collected);
    context.Set("q3.pickup_respawn_times", respawnTimes);
}

}  // namespace sdl3cpp::services::impl
