#include "services/interfaces/workflow/rendering/workflow_bsp_load_step.hpp"
#include "services/interfaces/workflow/rendering/pk3_bsp_loader.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <nlohmann/json.hpp>
#include <zip.h>

#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowBspLoadStep::WorkflowBspLoadStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBspLoadStep::GetPluginId() const {
    return "bsp.load";
}

void WorkflowBspLoadStep::Execute(const WorkflowStepDefinition& step,
                                  WorkflowContext& context) {
    const std::string pk3_path =
        GetStringParamOrInput(step, context, "pk3_path", "");

    // Context key q3.pending_map (set by menu) takes precedence over the
    // workflow parameter. This allows in-process map switching without
    // modifying the workflow JSON.
    std::string map_name;
    const auto* pendingMap = context.TryGet<std::string>("q3.pending_map");
    if (pendingMap && !pendingMap->empty()) {
        map_name = *pendingMap;
        context.Set<std::string>("q3.pending_map", "");
    } else {
        map_name = GetStringParamOrInput(step, context, "map_name", "q3dm7");
    }
    if (map_name.empty()) map_name = "q3dm7";

    WorkflowStepParameterResolver params;
    const auto* scaleParam = params.FindParameter(step, "scale");
    const float scale =
        (scaleParam && scaleParam->type == WorkflowParameterValue::Type::Number)
            ? static_cast<float>(scaleParam->numberValue)
            : 1.0f / 32.0f;

    if (pk3_path.empty()) {
        throw std::runtime_error("bsp.load: 'pk3_path' parameter required");
    }
    int zip_err    = 0;
    zip_t* archive = zip_open(pk3_path.c_str(), ZIP_RDONLY, &zip_err);
    if (!archive) {
        throw std::runtime_error("bsp.load: Failed to open pk3: " + pk3_path);
    }
    context.Set("q3.maps", ListPk3Maps(archive));
    // Closes `archive` on every path (success or throw).
    auto bspData = ReadBspFromPk3(archive, map_name, pk3_path);
    if (logger_) {
        logger_->Info("bsp.load: Read maps/" + map_name + ".bsp (" +
                      std::to_string(bspData->size()) + " bytes)");
    }
    ValidateBspHeader(*bspData);
    context.Set("bsp_raw_data", bspData);
    context.Set("bsp_config", nlohmann::json{{"pk3_path", pk3_path},
                                             {"map_name", map_name},
                                             {"scale", scale}});
    if (logger_) {
        logger_->Info("bsp.load: '" + map_name + "' validated, " +
                      std::to_string(bspData->size()) +
                      " bytes stored in context");
    }
}

}  // namespace sdl3cpp::services::impl
