#include "services/interfaces/app/app_bootstrap.hpp"

#include "services/interfaces/workflow/workflow_app_init_step.hpp"
#include "services/interfaces/workflow/workflow_load_workflow_step.hpp"

namespace sdl3cpp::services::app {

WorkflowRuntime BuildWorkflowRuntime(std::shared_ptr<ILogger> logger) {
    WorkflowRuntime runtime;
    runtime.registry  = std::make_shared<impl::WorkflowStepRegistry>();
    runtime.registrar = std::make_unique<impl::WorkflowRegistrar>(logger);
    runtime.registrar->RegisterSteps(runtime.registry);

    // Register application lifecycle steps.
    runtime.registry->RegisterStep(
        std::make_shared<impl::WorkflowAppInitStep>(logger));
    runtime.registry->RegisterStep(
        std::make_shared<impl::WorkflowLoadWorkflowStep>(logger));

    runtime.executor =
        std::make_shared<impl::WorkflowExecutor>(runtime.registry, logger);

    // Register executor-dependent steps (control.loop.while,
    // workflow.execute).
    runtime.registrar->RegisterExecutorSteps(runtime.registry,
                                             runtime.executor);
    return runtime;
}

}  // namespace sdl3cpp::services::app
