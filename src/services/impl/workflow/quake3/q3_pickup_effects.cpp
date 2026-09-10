#include "services/interfaces/workflow/quake3/q3_pickup_effects.hpp"

#include "services/interfaces/workflow/quake3/q3_pickup_apply.hpp"
#include "services/interfaces/workflow/quake3/q3_pickup_position.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr float kTouchRadius = 1.0f;

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
        const bool isAmmo   = HasClassPrefix(cls, "ammo_");
        const bool isWeapon = HasClassPrefix(cls, "weapon_");
        if (!isHealth && !isArmor && !isAmmo && !isWeapon) continue;

        const std::string id = ent.value("id", std::string{});
        if (id.empty()) continue;
        if (state.collected.value(id, false)) continue;

        glm::vec3 entPos(0.0f);
        if (!ReadEntityPosition(ent, entPos)) continue;
        if (glm::distance(playerPos, entPos) >= kTouchRadius) continue;

        const double respawnDelay = ApplyOnePickup(cls, state);
        state.collected[id]       = true;
        state.respawnTimes[id]    = elapsed + respawnDelay;

        if (logger) {
            logger->Info("q3.pickups.touch: collected " + cls + " (id=" + id +
                         ")");
        }
    }
}

}  // namespace sdl3cpp::services::impl
