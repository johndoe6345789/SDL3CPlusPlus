#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <nlohmann/json.hpp>

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

bool ReadVec3(const nlohmann::json& value, btVector3& out);

/**
 * @brief If `ent` is a trigger_push/trigger_teleport whose bounds the
 * player's AABB overlaps and whose cooldown has elapsed, applies its
 * effect (teleport or jump-pad launch) to `body`, updates `cooldowns`,
 * logs it, and returns true.
 *
 * On teleport, `playerAabbMin`/`playerAabbMax` are updated in place so
 * a later trigger this same frame sees the post-teleport position.
 */
bool TryActivateTrigger(const nlohmann::json& ent, const std::string& classname,
                        const std::string& id, btRigidBody* body,
                        const btVector3& playerPos, btVector3& playerAabbMin,
                        btVector3& playerAabbMax, uint32_t frame,
                        nlohmann::json& cooldowns,
                        const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
