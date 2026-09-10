#include "services/interfaces/workflow/quake3/q3_pickup_effects.hpp"

#include "services/interfaces/workflow/quake3/q3_pickup_apply.hpp"
#include "services/interfaces/workflow/quake3/q3_pickup_position.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_constants.hpp"

namespace sdl3cpp::services::impl {
namespace {

/// bg_public.h ITEM_RADIUS: an item is a 15-unit cube about its origin.
constexpr float kItemRadius = q3::FromQuakeUnits(15.0f);

/// Quake picks an item up when the player's bounding box overlaps the
/// item's, not when their centres are close. A radius about the origin
/// gets this wrong in both directions: the player's origin sits 24
/// units above their feet, so an item resting on the floor can be out
/// of range while the player is stood in it.
bool BoxesOverlap(const glm::vec3& playerPos, const glm::vec3& itemPos) {
    const glm::vec3 playerMins =
        playerPos + glm::vec3(-q3::kPlayerHalfWidth, q3::kPlayerFeet,
                              -q3::kPlayerHalfWidth);
    const glm::vec3 playerMaxs =
        playerPos +
        glm::vec3(q3::kPlayerHalfWidth, q3::kPlayerHead, q3::kPlayerHalfWidth);

    for (int axis = 0; axis < 3; ++axis) {
        if (playerMins[axis] > itemPos[axis] + kItemRadius ||
            playerMaxs[axis] < itemPos[axis] - kItemRadius) {
            return false;
        }
    }
    return true;
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
        const bool isAmmo   = HasClassPrefix(cls, "ammo_");
        const bool isWeapon = HasClassPrefix(cls, "weapon_");
        if (!isHealth && !isArmor && !isAmmo && !isWeapon) continue;

        const std::string id = ent.value("id", std::string{});
        if (id.empty()) continue;
        if (state.collected.value(id, false)) continue;

        glm::vec3 entPos(0.0f);
        if (!ReadEntityPosition(ent, entPos)) continue;
        if (!BoxesOverlap(playerPos, entPos)) continue;

        // Leave an item alone when it has nothing to give, rather than
        // consuming it and sending it away to respawn.
        if (!CanPickUpItem(cls, state)) continue;

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
