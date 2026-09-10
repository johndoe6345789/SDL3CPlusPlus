#include "services/interfaces/workflow_registrar.hpp"

#include "services/impl/workflow/workflow_registrar_categories.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_control_while_step.hpp"
#include "services/interfaces/workflow/workflow_execute_step.hpp"

#include <memory>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowRegistrar::WorkflowRegistrar(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

void WorkflowRegistrar::RegisterSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry) {
    if (!registry) return;

    // Services below are wired in later; steps needing them are
    // registered with nullptr and resolved by name once wired.
    std::shared_ptr<IInputService> inputSvc               = nullptr;
    std::shared_ptr<IAudioService> audioSvc               = nullptr;
    std::shared_ptr<ISceneService> sceneSvc               = nullptr;
    std::shared_ptr<IConfigService> configSvc             = nullptr;
    std::shared_ptr<IGraphicsService> graphicsSvc         = nullptr;
    std::shared_ptr<IShaderSystemRegistry> shaderRegistry = nullptr;

    // Each call below registers one slice; order must not change.
    using namespace registrar_detail;
    int count = 0;
    count += RegisterGraphicsInitSteps(registry, logger_);
    count += RegisterRenderingCoreSteps(registry, logger_);
    count += RegisterRenderingBspSteps(registry, logger_);
    count += RegisterRenderingPostfxSteps(registry, logger_);
    count += RegisterRenderingOverlaySteps(registry, logger_);
    count += RegisterRenderingQ3HudSteps(registry, logger_);
    count += RegisterRenderingQ3ModelSteps(registry, logger_);
    count += RegisterRenderingQ3PmoveSteps(registry, logger_);
    count += RegisterRenderingQ3CombatSteps(registry, logger_);
    count += RegisterGraphicsMiscSteps(registry, logger_);
    count += RegisterCameraSteps(registry, logger_);
    count += RegisterPhysicsSteps(registry, logger_);
    count += RegisterInputSteps(registry, logger_, inputSvc);
    count += RegisterAudioSteps(registry, logger_, audioSvc);
    count += RegisterControlSteps(registry, logger_);
    count += RegisterDataOpsCoreSteps(registry, logger_);
    count += RegisterListNumberSteps(registry, logger_);
    count += RegisterParticleStringValueVfxSteps(registry, logger_);
    count += RegisterModelSteps(registry, logger_);
    count += RegisterSceneSteps(registry, logger_, sceneSvc);
    count += RegisterCameraServiceSteps(registry, logger_, configSvc);
    count += RegisterSystemDataSteps(registry, logger_);
    count += RegisterMediaSteps(registry, logger_, configSvc, audioSvc);
    count +=
        RegisterShaderSteps(registry, logger_, graphicsSvc, shaderRegistry);
    count += RegisterExitStep(registry, logger_);

    if (logger_) {
        logger_->Info("WorkflowRegistrar: " + std::to_string(count) +
                      " base steps registered");
    }
}

void WorkflowRegistrar::RegisterExecutorSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<IWorkflowExecutor> executor) {
    if (!registry || !executor) return;

    registry->RegisterStep(
        std::make_shared<WorkflowControlWhileStep>(logger_, executor));
    registry->RegisterStep(
        std::make_shared<WorkflowExecuteStep>(logger_, executor));

    if (logger_) {
        logger_->Info(
            "WorkflowRegistrar: 2 executor-dependent steps registered");
    }
}

}  // namespace sdl3cpp::services::impl
