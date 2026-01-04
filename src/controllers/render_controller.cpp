#include "render_controller.hpp"
#include "../services/interfaces/i_logger.hpp"
#include "../services/interfaces/i_graphics_service.hpp"
#include "../services/interfaces/i_script_service.hpp"
#include "../services/interfaces/i_gui_service.hpp"
#include "../services/interfaces/i_scene_service.hpp"

namespace sdl3cpp::controllers {

RenderController::RenderController(di::ServiceRegistry& registry)
    : registry_(registry), logger_(registry.GetService<services::ILogger>()) {
    logger_->Trace("RenderController", "RenderController", "", "Created");
}

RenderController::~RenderController() {
    logger_->Trace("RenderController", "~RenderController", "", "Destroyed");
}

void RenderController::RenderFrame(float time) {
    logger_->Trace("RenderController::RenderFrame: Entering");

    // Get required services
    auto graphicsService = registry_.GetService<services::IGraphicsService>();
    auto scriptService = registry_.GetService<services::IScriptService>();
    auto guiService = registry_.GetService<services::IGuiService>();
    auto sceneService = registry_.GetService<services::ISceneService>();

    if (!graphicsService) {
        logger_->Error("RenderController::RenderFrame: Graphics service not available");
        logger_->Trace("RenderController::RenderFrame: Exiting");
        return;
    }

    // Begin frame
    graphicsService->BeginFrame();

    // Load scene and render
    if (scriptService && sceneService) {
        // Load scene objects from script
        auto sceneObjects = scriptService->LoadSceneObjects();
        sceneService->LoadScene(sceneObjects);
        
        // Get render commands from scene service
        auto renderCommands = sceneService->GetRenderCommands(time);

        // Get view-projection matrix
        float aspect = 1920.0f / 1080.0f;  // TODO: Get from window service
        auto viewProj = scriptService->GetViewProjectionMatrix(aspect);

        // Render scene
        graphicsService->RenderScene(renderCommands, viewProj);
    }

    // Render GUI overlay
    if (guiService && scriptService && scriptService->HasGuiCommands()) {
        auto guiCommands = scriptService->LoadGuiCommands();
        // guiService->PrepareFrame(guiCommands, width, height);
        // guiService->RenderToSwapchain(commandBuffer, swapchainImage);
    }

    // End frame and present
    graphicsService->EndFrame();

    logger_->Trace("RenderController::RenderFrame: Exiting");
}

}  // namespace sdl3cpp::controllers
