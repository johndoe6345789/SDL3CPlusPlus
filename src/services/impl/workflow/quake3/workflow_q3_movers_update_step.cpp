#include "services/interfaces/workflow/quake3/workflow_q3_movers_update_step.hpp"
#include "services/interfaces/workflow/quake3/q3_mover_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

WorkflowQ3MoversUpdateStep::WorkflowQ3MoversUpdateStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3MoversUpdateStep::GetPluginId() const { return "q3.movers.update"; }

void WorkflowQ3MoversUpdateStep::Execute(const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    const auto* moversPtr = context.TryGet<sdl3cpp::q3::MoverList>("q3.movers");
    if (!moversPtr || !(*moversPtr)) return;

    auto& movers = **moversPtr;
    if (movers.empty()) return;

    const float dt = (float)context.GetDouble("frame.delta_time", 0.016);
    if (dt <= 0.f) return;

    // Player position: try q3.player_pos first, fall back to camera.state
    glm::vec3 playerPos(0.f);
    if (const auto* pp = context.TryGet<glm::vec3>("q3.player_pos")) {
        playerPos = *pp;
    } else {
        const auto camState = context.Get<nlohmann::json>("camera.state", nlohmann::json::object());
        if (camState.contains("position") && camState["position"].is_array()) {
            const auto& cp = camState["position"];
            if (cp.size() >= 3)
                playerPos = {cp[0].get<float>(), cp[1].get<float>(), cp[2].get<float>()};
        }
    }

    glm::vec3 velocityPush(0.f);

    for (auto& m : movers) {
        using State = sdl3cpp::q3::Q3Mover::State;

        const glm::vec3 prevPos = m.currentPos;

        switch (m.state) {
            case State::AtPos1: {
                // Trigger: player within 2.5 units
                const glm::vec3 diff = playerPos - m.pos1;
                const float d2 = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
                if (d2 < 2.5f * 2.5f) {
                    m.state = State::MovingTo2;
                    m.stateProgress = 0.f;
                }
                m.currentPos = m.pos1;
                break;
            }
            case State::MovingTo2: {
                m.stateProgress += dt / std::max(m.travelTime, 0.001f);
                m.stateProgress = std::min(m.stateProgress, 1.f);
                m.currentPos = glm::mix(m.pos1, m.pos2, m.stateProgress);
                if (m.stateProgress >= 1.f) {
                    m.state = State::AtPos2;
                    m.stateTimer = m.waitTime;
                }
                break;
            }
            case State::AtPos2: {
                m.stateTimer -= dt;
                m.currentPos = m.pos2;
                if (m.stateTimer <= 0.f) {
                    m.state = State::MovingTo1;
                    m.stateProgress = 1.f;
                }
                break;
            }
            case State::MovingTo1: {
                m.stateProgress -= dt / std::max(m.travelTime, 0.001f);
                m.stateProgress = std::max(m.stateProgress, 0.f);
                m.currentPos = glm::mix(m.pos1, m.pos2, m.stateProgress);
                if (m.stateProgress <= 0.f) {
                    m.state = State::AtPos1;
                    m.currentPos = m.pos1;
                }
                break;
            }
        }

        // Compute velocity for this frame
        m.velocity = (m.currentPos - prevPos) / dt;

        // Push player if they are very close (within 1.5 units) and mover is moving
        const bool isMoving = (m.state == State::MovingTo2 || m.state == State::MovingTo1);
        if (isMoving) {
            const glm::vec3 diff = playerPos - m.currentPos;
            const float d2 = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
            if (d2 < 1.5f * 1.5f) {
                velocityPush += m.velocity;
            }
        }
    }

    // Write push back (additive with any existing push)
    const glm::vec3 existing = context.Get<glm::vec3>("q3.player_velocity_push", glm::vec3(0.f));
    context.Set("q3.player_velocity_push", existing + velocityPush);
}

}  // namespace sdl3cpp::services::impl
