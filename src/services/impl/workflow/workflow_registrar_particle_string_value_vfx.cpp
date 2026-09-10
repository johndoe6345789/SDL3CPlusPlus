#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/workflow_generic_steps/workflow_particle_emit_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_particle_update_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_concat_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_contains_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_equals_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_format_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_join_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_lower_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_replace_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_split_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_trim_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_upper_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_assert_exists_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_set_if_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_assert_type_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_clear_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_copy_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_default_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_literal_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_vfx_spawn_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_vfx_destroy_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterParticleStringValueVfxSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    int count = 0;

    // ── Particle ───────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowParticleEmitStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowParticleUpdateStep>(logger));
    count += 2;

    // ── String ─────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowStringConcatStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowStringContainsStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowStringEqualsStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowStringFormatStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowStringJoinStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowStringLowerStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowStringReplaceStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowStringSplitStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowStringTrimStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowStringUpperStep>(logger));
    count += 10;

    // ── Value ──────────────────────────────────────────────────
    registry->RegisterStep(
        std::make_shared<WorkflowValueAssertExistsStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowValueAssertTypeStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowValueClearStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowValueCopyStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowValueDefaultStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowValueLiteralStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowValueSetIfStep>(logger));
    count += 7;

    // ── VFX ────────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowVfxSpawnStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowVfxDestroyStep>(logger));
    count += 2;

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
