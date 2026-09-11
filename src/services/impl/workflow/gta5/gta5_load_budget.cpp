#include "services/interfaces/workflow/gta5/gta5_load_budget.hpp"

#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"

#include <string>

namespace sdl3cpp::services::impl {

Gta5LoadBudget ReadGta5LoadBudget(const WorkflowStepDefinition& step,
                                  const WorkflowContext& context,
                                  const Gta5StreamingConfig& streaming) {
    const bool loading =
        !context.Get<std::string>("gta5.loading.text", std::string())
             .empty();
    Gta5LoadBudget budget;
    if (loading) {
        budget.uploadMs = Gta5NumberOr(step, "upload_budget_ms", 8.f);
        budget.spawns = Gta5ParameterOrInt(step, "max_spawns_per_frame",
                                           streaming.maxSpawnsPerFrame);
    } else {
        budget.uploadMs = Gta5NumberOr(step, "stream_upload_budget_ms", 2.f);
        budget.spawns = Gta5ParameterOrInt(step, "stream_spawns_per_frame", 48);
    }
    if (budget.spawns <= 0) budget.spawns = 64;
    // Reading a tile's ymaps is done on this thread; a couple a frame
    // keeps the first frame from reading all of them at once.
    budget.reads = Gta5ParameterOrInt(step, "tile_reads_per_frame", 2);
    return budget;
}

}  // namespace sdl3cpp::services::impl
