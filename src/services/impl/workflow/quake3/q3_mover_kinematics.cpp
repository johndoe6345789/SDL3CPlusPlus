#include "services/interfaces/workflow/quake3/q3_mover_kinematics.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

glm::vec3 ResolveQ3PlayerPosition(WorkflowContext& context) {
    if (const auto* pp = context.TryGet<glm::vec3>("q3.player_pos")) {
        return *pp;
    }
    const auto camState =
        context.Get<nlohmann::json>("camera.state", nlohmann::json::object());
    if (camState.contains("position") && camState["position"].is_array()) {
        const auto& cp = camState["position"];
        if (cp.size() >= 3) {
            return {cp[0].get<float>(), cp[1].get<float>(), cp[2].get<float>()};
        }
    }
    return glm::vec3(0.f);
}

glm::vec3 UpdateQ3Mover(sdl3cpp::q3::Q3Mover& m, const glm::vec3& playerPos,
                        float dt) {
    using State = sdl3cpp::q3::Q3Mover::State;

    const glm::vec3 prevPos = m.currentPos;

    switch (m.state) {
        case State::AtPos1: {
            // Trigger: player within 2.5 units
            const glm::vec3 diff = playerPos - m.pos1;
            const float d2 =
                diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
            if (d2 < 2.5f * 2.5f) {
                m.state         = State::MovingTo2;
                m.stateProgress = 0.f;
            }
            m.currentPos = m.pos1;
            break;
        }
        case State::MovingTo2: {
            m.stateProgress += dt / std::max(m.travelTime, 0.001f);
            m.stateProgress = std::min(m.stateProgress, 1.f);
            m.currentPos    = glm::mix(m.pos1, m.pos2, m.stateProgress);
            if (m.stateProgress >= 1.f) {
                m.state      = State::AtPos2;
                m.stateTimer = m.waitTime;
            }
            break;
        }
        case State::AtPos2: {
            m.stateTimer -= dt;
            m.currentPos = m.pos2;
            if (m.stateTimer <= 0.f) {
                m.state         = State::MovingTo1;
                m.stateProgress = 1.f;
            }
            break;
        }
        case State::MovingTo1: {
            m.stateProgress -= dt / std::max(m.travelTime, 0.001f);
            m.stateProgress = std::max(m.stateProgress, 0.f);
            m.currentPos    = glm::mix(m.pos1, m.pos2, m.stateProgress);
            if (m.stateProgress <= 0.f) {
                m.state      = State::AtPos1;
                m.currentPos = m.pos1;
            }
            break;
        }
    }

    m.velocity = (m.currentPos - prevPos) / dt;

    // Push player if they are very close (within 1.5 units) and mover is
    // moving
    const bool isMoving =
        (m.state == State::MovingTo2 || m.state == State::MovingTo1);
    if (isMoving) {
        const glm::vec3 diff = playerPos - m.currentPos;
        const float d2 = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        if (d2 < 1.5f * 1.5f) {
            return m.velocity;
        }
    }
    return glm::vec3(0.f);
}

}  // namespace sdl3cpp::services::impl
