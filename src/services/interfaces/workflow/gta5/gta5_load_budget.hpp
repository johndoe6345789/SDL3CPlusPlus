#pragma once

#include "services/interfaces/workflow/gta5/gta5_config_types.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

namespace sdl3cpp::services::impl {

/// How much of a frame gta5.tiles.load may spend.
struct Gta5LoadBudget {
    float uploadMs{8.f};
    int spawns{64};
    int reads{2};
};

/// While the player is held for the ground (gta5.loading.text is set),
/// upload_budget_ms and max_spawns_per_frame: the wait matters more than
/// the frame rate. After, stream_upload_budget_ms (default 2) and
/// stream_spawns_per_frame (default 48): 8 ms of uploads a frame held a
/// drive into new tiles near 100 fps.
Gta5LoadBudget ReadGta5LoadBudget(const WorkflowStepDefinition& step,
                                  const WorkflowContext& context,
                                  const Gta5StreamingConfig& streaming);

}  // namespace sdl3cpp::services::impl
