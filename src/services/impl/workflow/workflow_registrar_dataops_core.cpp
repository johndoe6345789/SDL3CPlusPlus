#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/workflow_generic_steps/workflow_variable_set_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_variable_get_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_array_create_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_array_append_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_bool_and_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_bool_not_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_bool_or_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_compare_eq_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_compare_gt_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_compare_gte_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_compare_lt_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_compare_lte_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_compare_ne_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_debug_log_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_debug_metrics_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterDataOpsCoreSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    int count = 0;

    // ── Variables ──────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowVariableSetStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowVariableGetStep>(logger));
    count += 2;

    // ── Arrays ─────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowArrayCreateStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowArrayAppendStep>(logger));
    count += 2;

    // ── Bool ───────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowBoolAndStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowBoolNotStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowBoolOrStep>(logger));
    count += 3;

    // ── Compare ────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowCompareEqStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowCompareGtStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowCompareGteStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowCompareLtStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowCompareLteStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowCompareNeStep>(logger));
    count += 6;

    // ── Debug ──────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowDebugLogStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowDebugMetricsStep>(logger));
    count += 2;

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
