#include "render_coordinator_service.hpp"

namespace sdl3cpp::services::impl {

RenderCoordinatorService::RenderCoordinatorService(std::shared_ptr<ILogger> logger,
                                                   std::shared_ptr<IGraphicsService> graphicsService,
                                                   std::shared_ptr<ISceneScriptService> sceneScriptService,
                                                   std::shared_ptr<IGuiScriptService> guiScriptService,
                                                   std::shared_ptr<IGuiService> guiService,
                                                   std::shared_ptr<ISceneService> sceneService)
    : logger_(std::move(logger)),
      graphicsService_(std::move(graphicsService)),
      sceneScriptService_(std::move(sceneScriptService)),
      guiScriptService_(std::move(guiScriptService)),
      guiService_(std::move(guiService)),
      sceneService_(std::move(sceneService)) {
    if (logger_) {
        logger_->Trace("RenderCoordinatorService", "RenderCoordinatorService",
                       "graphicsService=" + std::string(graphicsService_ ? "set" : "null") +
                       ", sceneScriptService=" + std::string(sceneScriptService_ ? "set" : "null") +
                       ", guiScriptService=" + std::string(guiScriptService_ ? "set" : "null") +
                       ", guiService=" + std::string(guiService_ ? "set" : "null") +
                       ", sceneService=" + std::string(sceneService_ ? "set" : "null"),
                       "Created");
    }
}

void RenderCoordinatorService::RenderFrame(float time) {
    if (logger_) {
        logger_->Trace("RenderCoordinatorService", "RenderFrame", "time=" + std::to_string(time), "Entering");
    }

    if (!graphicsService_) {
        if (logger_) {
            logger_->Error("RenderCoordinatorService::RenderFrame: Graphics service not available");
            logger_->Trace("RenderCoordinatorService", "RenderFrame", "", "Exiting");
        }
        return;
    }

    graphicsService_->BeginFrame();

    if (sceneScriptService_ && sceneService_) {
        auto sceneObjects = sceneScriptService_->LoadSceneObjects();
        sceneService_->LoadScene(sceneObjects);

        auto renderCommands = sceneService_->GetRenderCommands(time);

        float aspect = 1920.0f / 1080.0f;
        auto viewProj = sceneScriptService_->GetViewProjectionMatrix(aspect);

        graphicsService_->RenderScene(renderCommands, viewProj);
    }

    if (guiService_ && guiScriptService_ && guiScriptService_->HasGuiCommands()) {
        auto guiCommands = guiScriptService_->LoadGuiCommands();
        (void)guiCommands;
    }

    graphicsService_->EndFrame();

    if (logger_) {
        logger_->Trace("RenderCoordinatorService", "RenderFrame", "", "Exiting");
    }
}

}  // namespace sdl3cpp::services::impl
