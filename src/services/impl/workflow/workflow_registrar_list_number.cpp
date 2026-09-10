#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_append_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_concat_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_count_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_filter_equals_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_filter_gt_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_literal_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_map_add_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_map_mul_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_reduce_max_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_reduce_min_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_reduce_sum_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_abs_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_add_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_clamp_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_div_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_max_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_min_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_mul_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_round_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_sub_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterListNumberSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                            std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    int count = 0;

    // ── List ───────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowListAppendStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowListConcatStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowListCountStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowListFilterEqualsStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowListFilterGtStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowListLiteralStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowListMapAddStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowListMapMulStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowListReduceMaxStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowListReduceMinStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowListReduceSumStep>(logger));
    count += 11;

    // ── Number ─────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowNumberAbsStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowNumberAddStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowNumberClampStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowNumberDivStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowNumberMaxStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowNumberMinStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowNumberMulStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowNumberRoundStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowNumberSubStep>(logger));
    count += 9;

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
