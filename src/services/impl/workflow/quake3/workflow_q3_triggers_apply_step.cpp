#include "services/interfaces/workflow/quake3/workflow_q3_triggers_apply_step.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>
#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

WorkflowQ3TriggersApplyStep::WorkflowQ3TriggersApplyStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3TriggersApplyStep::GetPluginId() const { return "q3.triggers.apply"; }

void WorkflowQ3TriggersApplyStep::Execute(const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    bool didSomething = false;

    // ----------------------------------------------------------------
    // Apply velocity override (jump pad launch)
    // ----------------------------------------------------------------
    if (const auto* velOverride = context.TryGet<glm::vec3>("q3.player_velocity_override")) {
        const glm::vec3 vel = *velOverride;

        // Update q3.ps if present
        auto* psPtr = context.TryGet<nlohmann::json>("q3.ps");
        if (psPtr) {
            nlohmann::json ps = *psPtr;
            ps["velocity"] = nlohmann::json::array({vel.x, vel.y, vel.z});
            context.Set("q3.ps", ps);
        }

        // Update player_pos is NOT changed by velocity override — the physics
        // step will integrate it next frame. Just propagate the velocity push.
        context.Set("q3.player_velocity_push",
            context.Get<glm::vec3>("q3.player_velocity_push", glm::vec3(0.f)) + vel);

        context.Remove("q3.player_velocity_override");
        context.Remove("q3.player_on_jump_pad");
        didSomething = true;
    }

    // ----------------------------------------------------------------
    // Apply teleport
    // ----------------------------------------------------------------
    if (const auto* dest = context.TryGet<glm::vec3>("q3.player_teleport_dest")) {
        const glm::vec3 destPos = *dest;

        context.Set("q3.player_pos", destPos);

        auto* psPtr = context.TryGet<nlohmann::json>("q3.ps");
        if (psPtr) {
            nlohmann::json ps = *psPtr;
            ps["origin"]   = nlohmann::json::array({destPos.x, destPos.y, destPos.z});
            ps["velocity"] = nlohmann::json::array({0.f, 0.f, 0.f});
            context.Set("q3.ps", ps);
        }

        // Also update camera.state origin so the renderer follows immediately
        auto camState = context.Get<nlohmann::json>("camera.state", nlohmann::json::object());
        camState["position"] = nlohmann::json::array({destPos.x, destPos.y, destPos.z});
        context.Set("camera.state", camState);

        context.Remove("q3.player_teleport_dest");
        didSomething = true;
    }

    (void)didSomething;
}

}  // namespace sdl3cpp::services::impl
