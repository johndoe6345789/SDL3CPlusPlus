#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_filter_equals_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowListFilterEqualsStep::WorkflowListFilterEqualsStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowListFilterEqualsStep::GetPluginId() const {
    return "list.filter.equals";
}

void WorkflowListFilterEqualsStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string listKey = resolver.GetRequiredInputKey(step, "list");
    const std::string valueKey = resolver.GetRequiredInputKey(step, "value");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "list");

    if (const auto* list = context.TryGet<std::vector<double>>(listKey)) {
        const auto* value = context.TryGet<double>(valueKey);
        if (!value) {
            throw std::runtime_error("list.filter.equals missing numeric value input '" + valueKey + "'");
        }
        std::vector<double> filtered;
        filtered.reserve(list->size());
        for (double entry : *list) {
            if (entry == *value) {
                filtered.push_back(entry);
            }
        }
        context.Set(outputKey, std::move(filtered));
        if (logger_) {
            logger_->Trace("WorkflowListFilterEqualsStep", "Execute",
                           "type=double, input=" + listKey +
                               ", output=" + outputKey,
                           "Filtered numeric list");
        }
        return;
    }

    if (const auto* list = context.TryGet<std::vector<std::string>>(listKey)) {
        const auto* value = context.TryGet<std::string>(valueKey);
        if (!value) {
            throw std::runtime_error("list.filter.equals missing string value input '" + valueKey + "'");
        }
        std::vector<std::string> filtered;
        filtered.reserve(list->size());
        for (const auto& entry : *list) {
            if (entry == *value) {
                filtered.push_back(entry);
            }
        }
        context.Set(outputKey, std::move(filtered));
        if (logger_) {
            logger_->Trace("WorkflowListFilterEqualsStep", "Execute",
                           "type=string, input=" + listKey +
                               ", output=" + outputKey,
                           "Filtered string list");
        }
        return;
    }

    throw std::runtime_error("list.filter.equals requires list input '" + listKey +
                             "' to be vector<double> or vector<string>");
}

}  // namespace sdl3cpp::services::impl
