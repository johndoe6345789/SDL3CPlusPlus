#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/rendering/workflow_overlay_fps_init_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_overlay_fps_upload_quad_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_overlay_fps_upload_text_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_overlay_fps_draw_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_debug_screenshot_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_ssao_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_bloom_extract_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_bloom_blur_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterRenderingOverlaySteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    int count = 0;

    // overlay.fps split into atomic steps; chain them in this order.
    registry->RegisterStep(
        std::make_shared<WorkflowOverlayFpsInitStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowOverlayFpsUploadQuadStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowOverlayFpsUploadTextStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowOverlayFpsDrawStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowDebugScreenshotStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowPostfxSsaoStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowPostfxBloomExtractStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowPostfxBloomBlurStep>(logger));

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
