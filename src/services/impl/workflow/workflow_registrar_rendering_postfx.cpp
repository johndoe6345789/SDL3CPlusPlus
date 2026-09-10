#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/rendering/workflow_shadow_setup_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_shadow_pass_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_render_prepare_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_setup_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_frame_begin_offscreen_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_frame_end_scene_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_composite_draw_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_overlay_fps_init_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_overlay_fps_upload_quad_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_overlay_fps_upload_text_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_overlay_fps_draw_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_gpu_screenshot_capture_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_gpu_command_buffer_submit_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterRenderingPostfxSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    int count = 0;

    registry->RegisterStep(std::make_shared<WorkflowShadowSetupStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowShadowPassStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowRenderPrepareStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowPostfxSetupStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowFrameBeginOffscreenStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowFrameEndSceneStep>(logger));
    // postfx.composite split into atomic steps; chain them in this order.
    registry->RegisterStep(
        std::make_shared<WorkflowPostfxCompositeDrawStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowPostfxOverlayFpsInitStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowPostfxOverlayFpsUploadQuadStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowPostfxOverlayFpsUploadTextStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowPostfxOverlayFpsDrawStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowGpuScreenshotCaptureStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowGpuCommandBufferSubmitStep>(logger));
    registry->RegisterStep(

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
