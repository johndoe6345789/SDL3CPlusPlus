#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/rendering/workflow_render_grid_setup_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_render_grid_draw_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_frame_begin_gpu_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_frame_draw_bodies_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_frame_end_gpu_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_draw_textured_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_lighting_setup_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_spotlight_setup_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_spotlight_update_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_model_load_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_draw_viewmodel_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_geometry_create_flashlight_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_map_load_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_draw_map_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_taa_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_draw_textured_box_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterRenderingCoreSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                               std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    int count = 0;

    // ── Rendering ──────────────────────────────────────────────
    registry->RegisterStep(
        std::make_shared<WorkflowRenderGridSetupStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowRenderGridDrawStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowFrameBeginGpuStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowFrameDrawBodiesStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowFrameEndGpuStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowDrawTexturedStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowDrawTexturedBoxStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowLightingSetupStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowSpotlightSetupStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowSpotlightUpdateStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowModelLoadStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowDrawViewmodelStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowGeometryCreateFlashlightStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowMapLoadStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowDrawMapStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowPostfxTaaStep>(logger));

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
