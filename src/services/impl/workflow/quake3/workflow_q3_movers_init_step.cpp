#include "services/interfaces/workflow/quake3/workflow_q3_movers_init_step.hpp"
#include "services/interfaces/workflow/quake3/q3_mover_init.hpp"
#include "services/interfaces/workflow/quake3/q3_mover_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>

#include <string>

namespace sdl3cpp::services::impl {

WorkflowQ3MoversInitStep::WorkflowQ3MoversInitStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3MoversInitStep::GetPluginId() const {
    return "q3.movers.init";
}

void WorkflowQ3MoversInitStep::Execute(const WorkflowStepDefinition&,
                                       WorkflowContext& context) {
    if (context.GetBool("q3.movers_initialized", false)) return;

    const auto* entities = context.TryGet<nlohmann::json>("bsp.entities");
    const sdl3cpp::q3::MoverList movers = BuildQ3MoversFromEntities(
        entities ? *entities : nlohmann::json::array());

    context.Set("q3.movers", movers);
    context.Set("q3.movers_initialized", true);

    if (logger_) {
        logger_->Info("q3.movers.init: created " +
                     std::to_string(movers->size()) + " movers");
    }
}

}  // namespace sdl3cpp::services::impl
