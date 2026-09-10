#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/quake3/workflow_q3_pm_crouch_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_sound_init_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_sound_load_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_sound_play_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_music_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_stats_init_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_player_commit_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_ground_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_friction_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_accelerate_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_jump_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_slide_move_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterRenderingQ3PmoveSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    int count = 0;

    registry->RegisterStep(std::make_shared<WorkflowQ3SoundInitStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3SoundLoadStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3SoundPlayStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3MusicStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3StatsInitStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowQ3PlayerCommitStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3PmCrouchStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3PmGroundStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3PmFrictionStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowQ3PmAccelerateStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3PmJumpStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3PmSlideMoveStep>(logger));

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
