#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/quake3/workflow_q3_md3_read_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_md3_parse_tags_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_md3_parse_anim_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_md3_upload_surfaces_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_md3_draw_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_bots_spawn_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_bots_update_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_bots_draw_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_player_sync_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_step_slide_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_frame_time_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterRenderingQ3ModelSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    int count = 0;

    // q3.md3.load split into atomic steps; chain them in this order.
    registry->RegisterStep(std::make_shared<WorkflowQ3Md3ReadStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowQ3Md3ParseTagsStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowQ3Md3ParseAnimStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowQ3Md3UploadSurfacesStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3Md3DrawStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3BotsSpawnStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3BotsUpdateStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3BotsDrawStep>(logger));
    // Q3 pmove
    registry->RegisterStep(std::make_shared<WorkflowQ3PlayerSyncStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowQ3PmStepSlideStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3FrameTimeStep>(logger));

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
