#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_literal_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowListLiteralStep::WorkflowListLiteralStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowListLiteralStep::GetPluginId() const {
    return "list.literal";
}

void WorkflowListLiteralStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver ioResolver;
    WorkflowStepParameterResolver parameterResolver;
    const std::string outputKey = ioResolver.GetRequiredOutputKey(step, "list");
    const auto& items = parameterResolver.GetRequiredParameter(step, "items");

    std::string typeHint;
    if (const auto* typeParam = parameterResolver.FindParameter(step, "type")) {
        if (typeParam->type != WorkflowParameterValue::Type::String) {
            throw std::runtime_error("list.literal parameter 'type' must be a string");
        }
        typeHint = typeParam->stringValue;
        std::transform(typeHint.begin(), typeHint.end(), typeHint.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    }

    switch (items.type) {
    case WorkflowParameterValue::Type::String:
        context.Set(outputKey, std::vector<std::string>{items.stringValue});
        break;
    case WorkflowParameterValue::Type::Number:
        context.Set(outputKey, std::vector<double>{items.numberValue});
        break;
    case WorkflowParameterValue::Type::StringList:
        if (items.stringList.empty() && typeHint == "number") {
            context.Set(outputKey, std::vector<double>{});
        } else {
            context.Set(outputKey, items.stringList);
        }
        break;
    case WorkflowParameterValue::Type::NumberList:
        if (items.numberList.empty() && typeHint == "string") {
            context.Set(outputKey, std::vector<std::string>{});
        } else {
            context.Set(outputKey, items.numberList);
        }
        break;
    default:
        throw std::runtime_error("list.literal parameter 'items' must be string or number list");
    }

    if (logger_) {
        logger_->Trace("WorkflowListLiteralStep", "Execute",
                       "output=" + outputKey,
                       "Set literal workflow list");
    }
}

}  // namespace sdl3cpp::services::impl
