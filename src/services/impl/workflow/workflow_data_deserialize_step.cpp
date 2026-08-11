#include "services/interfaces/workflow/workflow_data_deserialize_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowDataDeserializeStep::WorkflowDataDeserializeStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowDataDeserializeStep::GetPluginId() const {
    return "data.deserialize";
}

void WorkflowDataDeserializeStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string dataKey = resolver.GetRequiredInputKey(step, "data");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "deserialized");
    const std::string format = resolver.GetOptionalParameterValue(step, "format", "json");

    const auto* dataPtr = context.TryGet<std::string>(dataKey);
    if (!dataPtr) {
        throw std::runtime_error("data.deserialize missing input '" + dataKey + "'");
    }

    nlohmann::json deserialized;

    if (format == "json") {
        // JSON deserialization
        try {
            deserialized = nlohmann::json::parse(*dataPtr);
        } catch (const std::exception& e) {
            throw std::runtime_error("data.deserialize failed to parse JSON: " + std::string(e.what()));
        }
    } else if (format == "binary") {
        // Binary deserialization (msgpack or BSON)
        // For now, try to parse as JSON (in real implementation, use binary decoding)
        try {
            deserialized = nlohmann::json::parse(*dataPtr);
        } catch (const std::exception& e) {
            throw std::runtime_error("data.deserialize failed to parse binary: " + std::string(e.what()));
        }
    } else {
        throw std::runtime_error("data.deserialize unsupported format: " + format);
    }

    context.Set(outputKey, deserialized);

    if (logger_) {
        logger_->Trace("WorkflowDataDeserializeStep", "Execute",
                       "format=" + format + ", output_key=" + outputKey,
                       "Deserialized data object");
    }
}

}  // namespace sdl3cpp::services::impl
