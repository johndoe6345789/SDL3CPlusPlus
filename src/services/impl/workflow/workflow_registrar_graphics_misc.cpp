#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/graphics/workflow_texture_load_step.hpp"
#include "services/interfaces/workflow/geometry/workflow_geometry_create_plane_step.hpp"
#include "services/interfaces/workflow/compute/workflow_compute_tessellate_step.hpp"

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

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
