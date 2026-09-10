#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/quake3/workflow_q3_menu_update_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_weapon_update_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_hud_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_hud_head_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_crosshair_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_hitmarker_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_menu_frame_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_mapselect_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_overlay_sw_begin_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_overlay_sw_end_init_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_overlay_sw_end_upload_surface_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_overlay_sw_end_upload_quad_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_overlay_sw_end_draw_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_overlay_sw_end_blit_head_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_overlay_sw_end_screenshot_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pickups_draw_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterRenderingQ3HudSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    int count = 0;

    registry->RegisterStep(std::make_shared<WorkflowQ3MenuUpdateStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowQ3WeaponUpdateStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowQ3PickupsDrawStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowOverlaySwBeginStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3HudStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3HudHeadStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3CrosshairStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3HitmarkerStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3MenuFrameStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowQ3MapSelectStep>(logger));
    // overlay.sw.end split into atomic steps; chain them in this order.
    registry->RegisterStep(
        std::make_shared<WorkflowOverlaySwEndInitStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowOverlaySwEndUploadSurfaceStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowOverlaySwEndUploadQuadStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowOverlaySwEndDrawStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowOverlaySwEndBlitHeadStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowOverlaySwEndScreenshotStep>(logger));

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
