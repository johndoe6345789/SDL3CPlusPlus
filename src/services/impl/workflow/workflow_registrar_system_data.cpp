#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/workflow_cmdline_args_step.hpp"
#include "services/interfaces/workflow/workflow_data_deserialize_step.hpp"
#include "services/interfaces/workflow/workflow_data_serialize_step.hpp"
#include "services/interfaces/workflow/workflow_network_connect_step.hpp"
#include "services/interfaces/workflow/workflow_network_receive_step.hpp"
#include "services/interfaces/workflow/workflow_network_send_step.hpp"
#include "services/interfaces/workflow/workflow_state_clear_step.hpp"
#include "services/interfaces/workflow/workflow_state_load_step.hpp"
#include "services/interfaces/workflow/workflow_state_save_step.hpp"
#include "services/interfaces/workflow/workflow_graphics_init_device_step.hpp"
#include "services/interfaces/workflow/workflow_graphics_init_swapchain_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterSystemDataSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    int count = 0;

    // ── Cmdline / Data / Network / State (logger-only) ──────────
    registry->RegisterStep(std::make_shared<WorkflowCmdlineArgsStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowDataDeserializeStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowDataSerializeStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowNetworkConnectStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowNetworkReceiveStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowNetworkSendStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowStateClearStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowStateLoadStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowStateSaveStep>(logger));
    count += 9;

    // ── Graphics device/swapchain (logger-only constructor) ───
    registry->RegisterStep(
        std::make_shared<WorkflowGraphicsInitDeviceStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowGraphicsInitSwapchainStep>(logger));
    count += 2;

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
