#include "services/interfaces/workflow/workflow_data_serialize_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowDataSerializeStep::WorkflowDataSerializeStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowDataSerializeStep::GetPluginId() const {
    return "data.serialize";
}

void WorkflowDataSerializeStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string dataKey = resolver.GetRequiredInputKey(step, "data");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "serialized");
    const std::string format = resolver.GetOptionalParameterValue(step, "format", "json");
    const std::string pretty = resolver.GetOptionalParameterValue(step, "pretty", "false");

    const auto* dataPtr = context.TryGet<nlohmann::json>(dataKey);
    if (!dataPtr) {
        throw std::runtime_error("data.serialize missing input '" + dataKey + "'");
    }

    std::string serialized;

    if (format == "json") {
        // JSON serialization
        if (pretty == "true") {
            serialized = dataPtr->dump(2);  // Pretty print with 2-space indent
        } else {
            serialized = dataPtr->dump();   // Compact format
        }
    } else if (format == "binary") {
        // Binary serialization (use BSON or msgpack approach)
        // For now, use msgpack format
        serialized = dataPtr->dump();  // In real implementation, use binary encoding
    } else {
        throw std::runtime_error("data.serialize unsupported format: " + format);
    }

    context.Set(outputKey, serialized);

    if (logger_) {
        logger_->Trace("WorkflowDataSerializeStep", "Execute",
                       "format=" + format + ", output_key=" + outputKey,
                       "Serialized data object");
    }
}

}  // namespace sdl3cpp::services::impl
