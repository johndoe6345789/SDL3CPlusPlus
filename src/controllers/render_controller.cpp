#include "render_controller.hpp"
#include "../logging/logger.hpp"
#include "../services/interfaces/i_graphics_service.hpp"
#include "../services/interfaces/i_script_service.hpp"
#include "../services/interfaces/i_gui_service.hpp"
#include "../services/interfaces/i_scene_service.hpp"

namespace sdl3cpp::controllers {

RenderController::RenderController(di::ServiceRegistry& registry)
    : registry_(registry) {
    logging::TraceGuard trace;
}

RenderController::~RenderController() {
    logging::TraceGuard trace;
}

void RenderController::RenderFrame(float time) {
    logging::TraceGuard trace;

    // Get required services
    auto graphicsService = registry_.GetService<services::IGraphicsService>();
    auto scriptService = registry_.GetService<services::IScriptService>();
    auto guiService = registry_.GetService<services::IGuiService>();
    auto sceneService = registry_.GetService<services::ISceneService>();

    if (!graphicsService) {
        logging::Logger::GetInstance().Error("Graphics service not available");
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
}

}  // namespace sdl3cpp::controllers
