#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_poll_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_mouse_grab_step.hpp"
#include "services/interfaces/workflow/input/workflow_input_poll_all_step.hpp"
#include "services/interfaces/workflow/input/workflow_input_keyboard_poll_step.hpp"
#include "services/interfaces/workflow/input/workflow_input_axis_combine_step.hpp"
#include "services/interfaces/workflow/input/workflow_input_button_combine_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_key_pressed_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_gamepad_axis_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_gamepad_button_pressed_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_mouse_button_pressed_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_mouse_position_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterInputSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                       std::shared_ptr<ILogger> logger,
                       std::shared_ptr<IInputService> inputSvc) {
    if (!registry) return 0;

    int count = 0;

    // ── Input (logger-only) ────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowInputPollStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowInputMouseGrabStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowInputPollAllStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowInputKeyboardPollStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowInputAxisCombineStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowInputButtonCombineStep>(logger));
    count += 3;

    // ── Input (service-dependent, nullptr until wired) ─────────
    registry->RegisterStep(
        std::make_shared<WorkflowInputKeyPressedStep>(inputSvc, logger));
    registry->RegisterStep(
        std::make_shared<WorkflowInputGamepadAxisStep>(inputSvc, logger));
    registry->RegisterStep(
        std::make_shared<WorkflowInputGamepadButtonPressedStep>(inputSvc,
                                                                logger));
    registry->RegisterStep(
        std::make_shared<WorkflowInputMouseButtonPressedStep>(inputSvc,
                                                              logger));
    registry->RegisterStep(
        std::make_shared<WorkflowInputMousePositionStep>(inputSvc, logger));
    count += 5;

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
