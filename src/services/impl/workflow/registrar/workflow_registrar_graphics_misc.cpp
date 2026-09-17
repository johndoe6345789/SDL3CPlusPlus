#include "services/impl/workflow/registrar/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/graphics/workflow_texture_load_step.hpp"
#include "services/interfaces/workflow/geometry/workflow_geometry_create_plane_step.hpp"
#include "services/interfaces/workflow/compute/workflow_compute_tessellate_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_video_record_begin_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_video_record_end_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_video_record_stop_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterGraphicsMiscSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                              std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    int count = 0;

    // ── Texture ───────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowTextureLoadStep>(logger));
    count += 1;

    // ── Geometry (textured planes) ────────────────────────────
    registry->RegisterStep(
        std::make_shared<WorkflowGeometryCreatePlaneStep>(logger));
    count += 1;

    // ── Compute (tessellation) ────────────────────────────────
    registry->RegisterStep(
        std::make_shared<WorkflowComputeTessellateStep>(logger));
    count += 1;

    // ── Video recording ───────────────────────────────────────
    registry->RegisterStep(
        std::make_shared<WorkflowVideoRecordBeginStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowVideoRecordEndStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowVideoRecordStopStep>(logger));
    count += 3;

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
