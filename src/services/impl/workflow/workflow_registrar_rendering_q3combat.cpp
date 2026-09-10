#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/quake3/workflow_q3_weapon_select_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_weapon_fire_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_missiles_move_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_missiles_impact_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_ammo_init_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_damage_apply_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_bots_damage_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_player_death_check_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_player_respawn_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pickups_touch_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pickups_respawn_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_movers_init_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_movers_update_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_triggers_check_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_triggers_apply_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_nav_build_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterRenderingQ3CombatSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    int count = 0;

    // Q3 weapons + missiles
    registry->RegisterStep(
        std::make_shared<WorkflowQ3WeaponSelectStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3WeaponFireStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowQ3MissilesMoveStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowQ3MissilesImpactStep>(logger));
    // Q3 damage + pickups + ammo
    registry->RegisterStep(std::make_shared<WorkflowQ3AmmoInitStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3DamageApplyStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3BotsDamageStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowQ3PlayerDeathCheckStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowQ3PlayerRespawnStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowQ3PickupsTouchStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowQ3PickupsRespawnStep>(logger));
    // Q3 movers + triggers + nav
    registry->RegisterStep(std::make_shared<WorkflowQ3MoversInitStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowQ3MoversUpdateStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowQ3TriggersCheckStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowQ3TriggersApplyStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3NavBuildStep>(logger));
    count += 55;

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
