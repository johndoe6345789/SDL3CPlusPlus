#include "workflow_default_step_registrar.hpp"
#include "workflow_config_load_step.hpp"
#include "workflow_config_migration_step.hpp"
#include "workflow_config_schema_step.hpp"
#include "workflow_config_version_step.hpp"
#include "workflow_runtime_config_step.hpp"
#include "workflow_generic_steps/workflow_bool_and_step.hpp"
#include "workflow_generic_steps/workflow_bool_not_step.hpp"
#include "workflow_generic_steps/workflow_bool_or_step.hpp"
#include "workflow_generic_steps/workflow_compare_eq_step.hpp"
#include "workflow_generic_steps/workflow_compare_gte_step.hpp"
#include "workflow_generic_steps/workflow_compare_gt_step.hpp"
#include "workflow_generic_steps/workflow_compare_lte_step.hpp"
#include "workflow_generic_steps/workflow_compare_lt_step.hpp"
#include "workflow_generic_steps/workflow_compare_ne_step.hpp"
#include "workflow_generic_steps/workflow_list_append_step.hpp"
#include "workflow_generic_steps/workflow_list_concat_step.hpp"
#include "workflow_generic_steps/workflow_list_count_step.hpp"
#include "workflow_generic_steps/workflow_list_filter_equals_step.hpp"
#include "workflow_generic_steps/workflow_list_filter_gt_step.hpp"
#include "workflow_generic_steps/workflow_list_literal_step.hpp"
#include "workflow_generic_steps/workflow_list_map_add_step.hpp"
#include "workflow_generic_steps/workflow_list_map_mul_step.hpp"
#include "workflow_generic_steps/workflow_list_reduce_max_step.hpp"
#include "workflow_generic_steps/workflow_list_reduce_min_step.hpp"
#include "workflow_generic_steps/workflow_list_reduce_sum_step.hpp"
#include "workflow_generic_steps/workflow_number_abs_step.hpp"
#include "workflow_generic_steps/workflow_number_add_step.hpp"
#include "workflow_generic_steps/workflow_number_clamp_step.hpp"
#include "workflow_generic_steps/workflow_number_div_step.hpp"
#include "workflow_generic_steps/workflow_number_max_step.hpp"
#include "workflow_generic_steps/workflow_number_min_step.hpp"
#include "workflow_generic_steps/workflow_number_mul_step.hpp"
#include "workflow_generic_steps/workflow_number_round_step.hpp"
#include "workflow_generic_steps/workflow_number_sub_step.hpp"
#include "workflow_generic_steps/workflow_string_concat_step.hpp"
#include "workflow_generic_steps/workflow_string_contains_step.hpp"
#include "workflow_generic_steps/workflow_string_equals_step.hpp"
#include "workflow_generic_steps/workflow_string_join_step.hpp"
#include "workflow_generic_steps/workflow_string_lower_step.hpp"
#include "workflow_generic_steps/workflow_string_replace_step.hpp"
#include "workflow_generic_steps/workflow_string_split_step.hpp"
#include "workflow_generic_steps/workflow_string_trim_step.hpp"
#include "workflow_generic_steps/workflow_string_upper_step.hpp"
#include "workflow_generic_steps/workflow_value_assert_exists_step.hpp"
#include "workflow_generic_steps/workflow_value_assert_type_step.hpp"
#include "workflow_generic_steps/workflow_value_clear_step.hpp"
#include "workflow_generic_steps/workflow_value_copy_step.hpp"
#include "workflow_generic_steps/workflow_value_default_step.hpp"
#include "workflow_generic_steps/workflow_value_literal_step.hpp"

#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowDefaultStepRegistrar::WorkflowDefaultStepRegistrar(std::shared_ptr<ILogger> logger,
                                                           std::shared_ptr<IProbeService> probeService)
    : logger_(std::move(logger)),
      probeService_(std::move(probeService)) {}

void WorkflowDefaultStepRegistrar::RegisterUsedSteps(
    const WorkflowDefinition& workflow,
    const std::shared_ptr<IWorkflowStepRegistry>& registry) const {
    if (!registry) {
        throw std::runtime_error("WorkflowDefaultStepRegistrar: registry is null");
    }

    std::unordered_set<std::string> plugins;
    for (const auto& step : workflow.steps) {
        plugins.insert(step.plugin);
    }

    if (plugins.contains("config.load")) {
        registry->RegisterStep(std::make_shared<WorkflowConfigLoadStep>(logger_));
    }
    if (plugins.contains("config.version.validate")) {
        registry->RegisterStep(std::make_shared<WorkflowConfigVersionStep>(logger_));
    }
    if (plugins.contains("config.migrate")) {
        registry->RegisterStep(std::make_shared<WorkflowConfigMigrationStep>(logger_, probeService_));
    }
    if (plugins.contains("config.schema.validate")) {
        registry->RegisterStep(std::make_shared<WorkflowConfigSchemaStep>(logger_, probeService_));
    }
    if (plugins.contains("runtime.config.build")) {
        registry->RegisterStep(std::make_shared<WorkflowRuntimeConfigStep>(logger_));
    }
    if (plugins.contains("value.copy")) {
        registry->RegisterStep(std::make_shared<WorkflowValueCopyStep>(logger_));
    }
    if (plugins.contains("value.default")) {
        registry->RegisterStep(std::make_shared<WorkflowValueDefaultStep>(logger_));
    }
    if (plugins.contains("value.literal")) {
        registry->RegisterStep(std::make_shared<WorkflowValueLiteralStep>(logger_));
    }
    if (plugins.contains("value.clear")) {
        registry->RegisterStep(std::make_shared<WorkflowValueClearStep>(logger_));
    }
    if (plugins.contains("value.assert.exists")) {
        registry->RegisterStep(std::make_shared<WorkflowValueAssertExistsStep>(logger_));
    }
    if (plugins.contains("value.assert.type")) {
        registry->RegisterStep(std::make_shared<WorkflowValueAssertTypeStep>(logger_));
    }
    if (plugins.contains("number.add")) {
        registry->RegisterStep(std::make_shared<WorkflowNumberAddStep>(logger_));
    }
    if (plugins.contains("number.sub")) {
        registry->RegisterStep(std::make_shared<WorkflowNumberSubStep>(logger_));
    }
    if (plugins.contains("number.mul")) {
        registry->RegisterStep(std::make_shared<WorkflowNumberMulStep>(logger_));
    }
    if (plugins.contains("number.div")) {
        registry->RegisterStep(std::make_shared<WorkflowNumberDivStep>(logger_));
    }
    if (plugins.contains("number.min")) {
        registry->RegisterStep(std::make_shared<WorkflowNumberMinStep>(logger_));
    }
    if (plugins.contains("number.max")) {
        registry->RegisterStep(std::make_shared<WorkflowNumberMaxStep>(logger_));
    }
    if (plugins.contains("number.clamp")) {
        registry->RegisterStep(std::make_shared<WorkflowNumberClampStep>(logger_));
    }
    if (plugins.contains("number.abs")) {
        registry->RegisterStep(std::make_shared<WorkflowNumberAbsStep>(logger_));
    }
    if (plugins.contains("number.round")) {
        registry->RegisterStep(std::make_shared<WorkflowNumberRoundStep>(logger_));
    }
    if (plugins.contains("compare.eq")) {
        registry->RegisterStep(std::make_shared<WorkflowCompareEqStep>(logger_));
    }
    if (plugins.contains("compare.ne")) {
        registry->RegisterStep(std::make_shared<WorkflowCompareNeStep>(logger_));
    }
    if (plugins.contains("compare.gt")) {
        registry->RegisterStep(std::make_shared<WorkflowCompareGtStep>(logger_));
    }
    if (plugins.contains("compare.gte")) {
        registry->RegisterStep(std::make_shared<WorkflowCompareGteStep>(logger_));
    }
    if (plugins.contains("compare.lt")) {
        registry->RegisterStep(std::make_shared<WorkflowCompareLtStep>(logger_));
    }
    if (plugins.contains("compare.lte")) {
        registry->RegisterStep(std::make_shared<WorkflowCompareLteStep>(logger_));
    }
    if (plugins.contains("bool.and")) {
        registry->RegisterStep(std::make_shared<WorkflowBoolAndStep>(logger_));
    }
    if (plugins.contains("bool.or")) {
        registry->RegisterStep(std::make_shared<WorkflowBoolOrStep>(logger_));
    }
    if (plugins.contains("bool.not")) {
        registry->RegisterStep(std::make_shared<WorkflowBoolNotStep>(logger_));
    }
    if (plugins.contains("string.concat")) {
        registry->RegisterStep(std::make_shared<WorkflowStringConcatStep>(logger_));
    }
    if (plugins.contains("string.trim")) {
        registry->RegisterStep(std::make_shared<WorkflowStringTrimStep>(logger_));
    }
    if (plugins.contains("string.lower")) {
        registry->RegisterStep(std::make_shared<WorkflowStringLowerStep>(logger_));
    }
    if (plugins.contains("string.upper")) {
        registry->RegisterStep(std::make_shared<WorkflowStringUpperStep>(logger_));
    }
    if (plugins.contains("string.replace")) {
        registry->RegisterStep(std::make_shared<WorkflowStringReplaceStep>(logger_));
    }
    if (plugins.contains("string.contains")) {
        registry->RegisterStep(std::make_shared<WorkflowStringContainsStep>(logger_));
    }
    if (plugins.contains("string.equals")) {
        registry->RegisterStep(std::make_shared<WorkflowStringEqualsStep>(logger_));
    }
    if (plugins.contains("string.split")) {
        registry->RegisterStep(std::make_shared<WorkflowStringSplitStep>(logger_));
    }
    if (plugins.contains("string.join")) {
        registry->RegisterStep(std::make_shared<WorkflowStringJoinStep>(logger_));
    }
    if (plugins.contains("list.literal")) {
        registry->RegisterStep(std::make_shared<WorkflowListLiteralStep>(logger_));
    }
    if (plugins.contains("list.append")) {
        registry->RegisterStep(std::make_shared<WorkflowListAppendStep>(logger_));
    }
    if (plugins.contains("list.concat")) {
        registry->RegisterStep(std::make_shared<WorkflowListConcatStep>(logger_));
    }
    if (plugins.contains("list.filter.equals")) {
        registry->RegisterStep(std::make_shared<WorkflowListFilterEqualsStep>(logger_));
    }
    if (plugins.contains("list.filter.gt")) {
        registry->RegisterStep(std::make_shared<WorkflowListFilterGtStep>(logger_));
    }
    if (plugins.contains("list.map.add")) {
        registry->RegisterStep(std::make_shared<WorkflowListMapAddStep>(logger_));
    }
    if (plugins.contains("list.map.mul")) {
        registry->RegisterStep(std::make_shared<WorkflowListMapMulStep>(logger_));
    }
    if (plugins.contains("list.reduce.sum")) {
        registry->RegisterStep(std::make_shared<WorkflowListReduceSumStep>(logger_));
    }
    if (plugins.contains("list.reduce.min")) {
        registry->RegisterStep(std::make_shared<WorkflowListReduceMinStep>(logger_));
    }
    if (plugins.contains("list.reduce.max")) {
        registry->RegisterStep(std::make_shared<WorkflowListReduceMaxStep>(logger_));
    }
    if (plugins.contains("list.count")) {
        registry->RegisterStep(std::make_shared<WorkflowListCountStep>(logger_));
    }
}

}  // namespace sdl3cpp::services::impl
