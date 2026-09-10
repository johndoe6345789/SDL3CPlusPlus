#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/rendering/workflow_bsp_load_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_lightmap_atlas_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_parse_spawn_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_entity_update_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_portal_view_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_tessellate_patches_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_build_polygons_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_flatten_geometry_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_extract_textures_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_upload_geometry_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_build_collision_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_spawn_apply_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterRenderingBspSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                              std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    int count = 0;

    registry->RegisterStep(std::make_shared<WorkflowBspLoadStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowBspLightmapAtlasStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowBspParseSpawnStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowBspEntityUpdateStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowBspPortalViewStep>(logger));
    // bsp.build_geometry split into atomic steps; chain them in this order.
    registry->RegisterStep(
        std::make_shared<WorkflowBspTessellatePatchesStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowBspBuildPolygonsStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowBspFlattenGeometryStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowBspExtractTexturesStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowBspUploadGeometryStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowBspBuildCollisionStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowSpawnApplyStep>(logger));

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
