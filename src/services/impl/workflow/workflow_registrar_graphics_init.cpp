#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/workflow_sdl_init_step.hpp"
#include "services/interfaces/workflow/workflow_sdl_window_create_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_graphics_init_viewport_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_graphics_init_renderer_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_graphics_gpu_init_step.hpp"
#include "services/interfaces/workflow/geometry/workflow_geometry_create_cube_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_gpu_shader_compile_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_gpu_pipeline_create_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_graphics_buffer_create_vertex_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_graphics_buffer_create_index_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_graphics_draw_submit_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_graphics_frame_begin_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_graphics_frame_end_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_graphics_screenshot_request_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterGraphicsInitSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                              std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    int count = 0;

    // ── SDL3 platform ──────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowSdlInitStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowSdlWindowCreateStep>(logger));
    count += 2;

    // ── GPU initialization ─────────────────────────────────────
    registry->RegisterStep(
        std::make_shared<WorkflowGraphicsInitViewportStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowGraphicsInitRendererStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowGraphicsGpuInitStep>(logger, nullptr));
    count += 3;

    // ── Graphics pipeline ──────────────────────────────────────
    registry->RegisterStep(
        std::make_shared<WorkflowGeometryCreateCubeStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowGpuShaderCompileStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowGpuPipelineCreateStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowGraphicsBufferCreateVertexStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowGraphicsBufferCreateIndexStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowGraphicsDrawSubmitStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowGraphicsFrameBeginStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowGraphicsFrameEndStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowGraphicsScreenshotRequestStep>(logger));
    count += 9;

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
