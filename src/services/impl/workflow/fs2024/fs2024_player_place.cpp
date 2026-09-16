#include "services/interfaces/workflow/fs2024/fs2024_player_place.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

float Fs2024YawForHeading(float headingDegrees) {
    // Forward (-sin yaw, -cos yaw) against compass (sin h, -cos h).
    return -headingDegrees * 3.14159265f / 180.f;
}

glm::vec3 Fs2024StandingOrigin(const Fs2024Heightfield& field, float x,
                               float z, float clearance) {
    const float ground = Fs2024HeightAt(field, x, z);
    return {x, ground - q3::kPlayerFeet + clearance, z};
}

bool Fs2024KeepOnGround(const Fs2024Heightfield& field,
                        Q3PlayerState& player, float margin) {
    const float lowX = field.origin.x + margin;
    const float lowZ = field.origin.y + margin;
    const float highX = field.origin.x - margin +
                        field.spacing * static_cast<float>(field.columns - 1);
    const float highZ = field.origin.y - margin +
                        field.spacing * static_cast<float>(field.rows - 1);
    glm::vec3& at = player.origin;
    bool moved = false;
    if (at.x < lowX || at.x > highX || at.z < lowZ || at.z > highZ) {
        at.x = std::clamp(at.x, lowX, highX);
        at.z = std::clamp(at.z, lowZ, highZ);
        player.velocity.x = 0.f;
        player.velocity.z = 0.f;
        moved = true;
    }

    // Resting on the ground, a box's flat bottom is never below the
    // ground under its centre; well below it means it went through.
    constexpr float kSunkTolerance = 0.3f;
    const float ground = Fs2024HeightAt(field, at.x, at.z);
    if (at.y + player.mins.y < ground - kSunkTolerance ||
        !std::isfinite(at.y)) {
        at.y = ground - player.mins.y + 0.05f;
        player.velocity.y = std::max(player.velocity.y, 0.f);
        if (!std::isfinite(player.velocity.y)) player.velocity = {};
        moved = true;
    }
    return moved;
}

void Fs2024MoveBody(btRigidBody* body, const glm::vec3& origin) {
    if (!body) return;
    btTransform transform = body->getWorldTransform();
    transform.setOrigin(btVector3(origin.x, origin.y, origin.z));
    body->setWorldTransform(transform);
    if (auto* motion = body->getMotionState()) {
        motion->setWorldTransform(transform);
    }
    body->setLinearVelocity(btVector3(0, 0, 0));
    body->setAngularVelocity(btVector3(0, 0, 0));
}

}  // namespace sdl3cpp::services::impl
