#include "render_coordinator_service.hpp"

namespace sdl3cpp::services::impl {

RenderCoordinatorService::RenderCoordinatorService(std::shared_ptr<ILogger> logger,
                                                   std::shared_ptr<IConfigService> configService,
                                                   std::shared_ptr<IGraphicsService> graphicsService,
                                                   std::shared_ptr<ISceneScriptService> sceneScriptService,
                                                   std::shared_ptr<IShaderScriptService> shaderScriptService,
                                                   std::shared_ptr<IGuiScriptService> guiScriptService,
                                                   std::shared_ptr<IGuiService> guiService,
                                                   std::shared_ptr<ISceneService> sceneService)
    : logger_(std::move(logger)),
      configService_(std::move(configService)),
      graphicsService_(std::move(graphicsService)),
      sceneScriptService_(std::move(sceneScriptService)),
      shaderScriptService_(std::move(shaderScriptService)),
      guiScriptService_(std::move(guiScriptService)),
      guiService_(std::move(guiService)),
      sceneService_(std::move(sceneService)) {
    if (logger_) {
        logger_->Trace("RenderCoordinatorService", "RenderCoordinatorService",
                       "configService=" + std::string(configService_ ? "set" : "null") +
                       ", graphicsService=" + std::string(graphicsService_ ? "set" : "null") +
                       ", sceneScriptService=" + std::string(sceneScriptService_ ? "set" : "null") +
                       ", shaderScriptService=" + std::string(shaderScriptService_ ? "set" : "null") +
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

    const bool useLuaScene = !configService_ || configService_->GetSceneSource() == SceneSource::Lua;
    if (!useLuaScene && !configFirstLogged_) {
        if (logger_) {
            logger_->Warn("RenderCoordinatorService::RenderFrame: config-first scene source selected; Lua scene path disabled");
        }
        configFirstLogged_ = true;
    }

    if (!shadersLoaded_) {
        if (!useLuaScene) {
            shadersLoaded_ = true;
        } else if (!shaderScriptService_) {
            if (logger_) {
                logger_->Error("RenderCoordinatorService::RenderFrame: Shader script service not available");
            }
            return;
        }
        if (!shadersLoaded_) {
            if (logger_) {
                logger_->Trace("RenderCoordinatorService", "RenderFrame",
                               "Priming bgfx with a dummy frame before shader load");
            }
            if (!graphicsService_->BeginFrame()) {
                if (logger_) {
                    logger_->Warn("RenderCoordinatorService::RenderFrame: Swapchain out of date during shader pre-frame");
                }
                graphicsService_->RecreateSwapchain();
                return;
            }
            if (!graphicsService_->EndFrame()) {
                if (logger_) {
                    logger_->Warn("RenderCoordinatorService::RenderFrame: Swapchain out of date during shader pre-frame");
                }
                graphicsService_->RecreateSwapchain();
                return;
            }
            if (logger_) {
                logger_->Trace("RenderCoordinatorService", "RenderFrame", "Loading shaders from Lua");
            }
            auto shaders = shaderScriptService_->LoadShaderPathsMap();
            graphicsService_->LoadShaders(shaders);
            shadersLoaded_ = true;
        }
    }

    if (!graphicsService_->BeginFrame()) {
        if (logger_) {
            logger_->Warn("RenderCoordinatorService::RenderFrame: Swapchain out of date during BeginFrame");
        }
        graphicsService_->RecreateSwapchain();
        return;
    }

    if (guiService_ && guiScriptService_ && guiScriptService_->HasGuiCommands()) {
        auto guiCommands = guiScriptService_->LoadGuiCommands();
        auto extent = graphicsService_->GetSwapchainExtent();
        guiService_->PrepareFrame(guiCommands, extent.first, extent.second);
    }

    if (useLuaScene && sceneScriptService_ && sceneService_) {
        auto sceneObjects = sceneScriptService_->LoadSceneObjects();
        sceneService_->LoadScene(sceneObjects);

        const auto& vertices = sceneService_->GetCombinedVertices();
        const auto& indices = sceneService_->GetCombinedIndices();
        bool geometryChanged = vertices.size() != lastVertexCount_ || indices.size() != lastIndexCount_;
        if (!vertices.empty() && !indices.empty() && (!geometryUploaded_ || geometryChanged)) {
            if (logger_) {
                logger_->Trace("RenderCoordinatorService", "RenderFrame",
                               "Uploading geometry vertices=" + std::to_string(vertices.size()) +
                               ", indices=" + std::to_string(indices.size()));
            }
            graphicsService_->UploadVertexData(vertices);
            graphicsService_->UploadIndexData(indices);
            geometryUploaded_ = true;
            lastVertexCount_ = vertices.size();
            lastIndexCount_ = indices.size();
        } else if (logger_) {
            logger_->Trace("RenderCoordinatorService", "RenderFrame",
                           "Geometry upload skipped (vertices=" + std::to_string(vertices.size()) +
                           ", indices=" + std::to_string(indices.size()) +
                           ", uploaded=" + std::string(geometryUploaded_ ? "true" : "false") +
                           ", changed=" + std::string(geometryChanged ? "true" : "false") + ")");
        }

        auto renderCommands = sceneService_->GetRenderCommands(time);

        auto extent = graphicsService_->GetSwapchainExtent();
        float aspect = extent.second == 0 ? 1.0f
                                          : static_cast<float>(extent.first) / static_cast<float>(extent.second);
        auto viewState = sceneScriptService_->GetViewState(aspect);

        graphicsService_->RenderScene(renderCommands, viewState);
    }

    if (!graphicsService_->EndFrame()) {
        if (logger_) {
            logger_->Warn("RenderCoordinatorService::RenderFrame: Swapchain out of date during EndFrame");
        }
        graphicsService_->RecreateSwapchain();
        return;
    }

    if (logger_) {
        logger_->Trace("RenderCoordinatorService", "RenderFrame", "", "Exiting");
    }
}

}  // namespace sdl3cpp::services::impl
