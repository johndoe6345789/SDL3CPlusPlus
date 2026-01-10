#include "frame_workflow_service.hpp"

#include "frame_workflow_step_registrar.hpp"
#include "../workflow_definition_parser.hpp"
#include "services/interfaces/i_logger.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

FrameWorkflowService::FrameWorkflowService(std::shared_ptr<ILogger> logger,
                                           std::shared_ptr<IConfigService> configService,
                                           std::shared_ptr<IAudioService> audioService,
                                           std::shared_ptr<IInputService> inputService,
                                           std::shared_ptr<IMeshService> meshService,
                                           std::shared_ptr<IPhysicsService> physicsService,
                                           std::shared_ptr<ISceneService> sceneService,
                                           std::shared_ptr<IRenderCoordinatorService> renderService,
                                           std::shared_ptr<IValidationTourService> validationTourService,
                                           std::shared_ptr<ISoundboardStateService> soundboardStateService,
                                           const std::filesystem::path& templatePath)
    : registry_(std::make_shared<WorkflowStepRegistry>()),
      executor_(registry_, logger),
      logger_(std::move(logger)) {
    std::filesystem::path path = templatePath.empty()
        ? std::filesystem::current_path() / "config" / "workflows" / "templates" / "frame_default.json"
        : templatePath;

    WorkflowDefinitionParser parser;
    workflow_ = parser.ParseFile(path);

    FrameWorkflowStepRegistrar registrar(logger_,
                                         std::move(configService),
                                         std::move(audioService),
                                         std::move(inputService),
                                         std::move(meshService),
                                         std::move(physicsService),
                                         std::move(sceneService),
                                         std::move(renderService),
                                         std::move(validationTourService),
                                         std::move(soundboardStateService));
    registrar.RegisterUsedSteps(workflow_, registry_);
}

void FrameWorkflowService::ExecuteFrame(float deltaTime, float elapsedTime) {
    if (logger_) {
        logger_->Trace("FrameWorkflowService", "ExecuteFrame",
                       "delta=" + std::to_string(deltaTime) +
                       ", elapsed=" + std::to_string(elapsedTime),
                       "Running frame workflow");
    }

    WorkflowContext context;
    context.Set("frame.delta", static_cast<double>(deltaTime));
    context.Set("frame.elapsed", static_cast<double>(elapsedTime));
    executor_.Execute(workflow_, context);
}

}  // namespace sdl3cpp::services::impl
