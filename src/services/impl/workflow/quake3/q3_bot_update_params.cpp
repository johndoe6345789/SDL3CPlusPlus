#include "services/interfaces/workflow/quake3/q3_bot_update_params.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {
namespace {

float NumberParameter(const WorkflowStepParameterResolver& params,
                      const WorkflowStepDefinition& step, const char* key,
                      float fallback) {
    const auto* p       = params.FindParameter(step, key);
    const bool isNumber = p && p->type == WorkflowParameterValue::Type::Number;
    return isNumber ? static_cast<float>(p->numberValue) : fallback;
}

}  // namespace

BotUpdateParams ReadBotUpdateParams(const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;
    BotUpdateParams p;
    p.chaseRange = NumberParameter(params, step, "chase_range", p.chaseRange);
    p.shootRange = NumberParameter(params, step, "shoot_range", p.shootRange);
    p.moveSpeed  = NumberParameter(params, step, "move_speed", p.moveSpeed);
    p.legIdle    = static_cast<int>(NumberParameter(
        params, step, "leg_idle", static_cast<float>(p.legIdle)));
    p.legRun     = static_cast<int>(
        NumberParameter(params, step, "leg_run", static_cast<float>(p.legRun)));
    p.legRunCount         = static_cast<int>(NumberParameter(
        params, step, "leg_run_cnt", static_cast<float>(p.legRunCount)));
    p.torsoStand          = static_cast<int>(NumberParameter(
        params, step, "torso_stand", static_cast<float>(p.torsoStand)));
    p.torsoAttack         = static_cast<int>(NumberParameter(
        params, step, "torso_attack", static_cast<float>(p.torsoAttack)));
    p.torsoAttackCount    = static_cast<int>(NumberParameter(
        params, step, "torso_atk_cnt", static_cast<float>(p.torsoAttackCount)));
    p.shootIntervalFrames = static_cast<int>(
        NumberParameter(params, step, "shoot_interval",
                        static_cast<float>(p.shootIntervalFrames)));
    p.replanFrames = static_cast<int>(NumberParameter(
        params, step, "replan_frames", static_cast<float>(p.replanFrames)));
    return p;
}

}  // namespace sdl3cpp::services::impl
