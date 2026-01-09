#include "render_coordinator_service.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace sdl3cpp::services::impl {

RenderCoordinatorService::RenderCoordinatorService(std::shared_ptr<ILogger> logger,
                                                   std::shared_ptr<IConfigService> configService,
                                                   std::shared_ptr<IConfigCompilerService> configCompilerService,
                                                   std::shared_ptr<IGraphicsService> graphicsService,
                                                   std::shared_ptr<ISceneScriptService> sceneScriptService,
                                                   std::shared_ptr<IShaderScriptService> shaderScriptService,
                                                   std::shared_ptr<IGuiScriptService> guiScriptService,
                                                   std::shared_ptr<IGuiService> guiService,
                                                   std::shared_ptr<ISceneService> sceneService,
                                                   std::shared_ptr<IValidationTourService> validationTourService)
    : logger_(std::move(logger)),
      configService_(std::move(configService)),
      configCompilerService_(std::move(configCompilerService)),
      graphicsService_(std::move(graphicsService)),
      sceneScriptService_(std::move(sceneScriptService)),
      shaderScriptService_(std::move(shaderScriptService)),
      guiScriptService_(std::move(guiScriptService)),
      guiService_(std::move(guiService)),
      sceneService_(std::move(sceneService)),
      validationTourService_(std::move(validationTourService)) {
    if (logger_) {
        logger_->Trace("RenderCoordinatorService", "RenderCoordinatorService",
                       "configService=" + std::string(configService_ ? "set" : "null") +
                       ", configCompilerService=" + std::string(configCompilerService_ ? "set" : "null") +
                       ", graphicsService=" + std::string(graphicsService_ ? "set" : "null") +
                       ", sceneScriptService=" + std::string(sceneScriptService_ ? "set" : "null") +
                       ", shaderScriptService=" + std::string(shaderScriptService_ ? "set" : "null") +
                       ", guiScriptService=" + std::string(guiScriptService_ ? "set" : "null") +
                       ", guiService=" + std::string(guiService_ ? "set" : "null") +
                       ", sceneService=" + std::string(sceneService_ ? "set" : "null") +
                       ", validationTourService=" + std::string(validationTourService_ ? "set" : "null"),
                       "Created");
    }
}

void RenderCoordinatorService::ConfigureRenderGraphPasses() {
    if (!graphicsService_) {
        return;
    }
    if (!configCompilerService_) {
        if (logger_) {
            logger_->Warn("RenderCoordinatorService::ConfigureRenderGraphPasses: Config compiler unavailable");
        }
        return;
    }

    const auto& result = configCompilerService_->GetLastResult();
    const auto& passes = result.renderGraph.passes;
    if (passes.empty()) {
        return;
    }

    std::vector<std::string> passOrder = result.renderGraphBuild.passOrder;
    if (passOrder.empty()) {
        passOrder.reserve(passes.size());
        for (const auto& pass : passes) {
            passOrder.push_back(pass.id);
        }
    }

    std::unordered_map<std::string, const RenderPassIR*> passMap;
    passMap.reserve(passes.size());
    for (const auto& pass : passes) {
        passMap.emplace(pass.id, &pass);
    }

    std::unordered_set<uint16_t> usedViewIds;
    usedViewIds.reserve(passes.size());
    for (const auto& pass : passes) {
        if (pass.hasViewId && pass.viewId >= 0) {
            usedViewIds.insert(static_cast<uint16_t>(pass.viewId));
        }
    }

    uint16_t nextAutoViewId = 0;
    auto reserveAutoViewId = [&]() -> uint16_t {
        while (usedViewIds.find(nextAutoViewId) != usedViewIds.end()) {
            ++nextAutoViewId;
        }
        const uint16_t selected = nextAutoViewId;
        usedViewIds.insert(selected);
        ++nextAutoViewId;
        return selected;
    };

    if (logger_) {
        logger_->Trace("RenderCoordinatorService", "ConfigureRenderGraphPasses",
                       "passes=" + std::to_string(passes.size()) +
                           ", passOrder=" + std::to_string(passOrder.size()));
    }

    for (const auto& passId : passOrder) {
        auto passIt = passMap.find(passId);
        if (passIt == passMap.end()) {
            if (logger_) {
                logger_->Warn("RenderCoordinatorService::ConfigureRenderGraphPasses: Pass not found id=" + passId);
            }
            continue;
        }
        const RenderPassIR& pass = *passIt->second;
        const uint16_t viewId = pass.hasViewId
            ? static_cast<uint16_t>(pass.viewId)
            : reserveAutoViewId();

        ViewClearConfig clearConfig;
        clearConfig.enabled = pass.clear.enabled;
        clearConfig.clearColor = pass.clear.clearColor;
        clearConfig.clearDepth = pass.clear.clearDepth;
        clearConfig.clearStencil = pass.clear.clearStencil;
        clearConfig.color = pass.clear.color;
        clearConfig.depth = pass.clear.depth;
        clearConfig.stencil = static_cast<uint8_t>(std::clamp(pass.clear.stencil, 0, 255));

        if (logger_) {
            logger_->Trace("RenderCoordinatorService", "ConfigureRenderGraphPasses",
                           "passId=" + pass.id + ", viewId=" + std::to_string(viewId) +
                               ", clear=" + std::string(clearConfig.enabled ? "true" : "false"));
        }

        graphicsService_->ConfigureView(viewId, clearConfig);
    }
}

void RenderCoordinatorService::RenderFrame(float time) {
    if (logger_) {
        logger_->Trace("RenderCoordinatorService", "RenderFrame", "time=" + std::to_string(time), "Entering");
    }

    ValidationFramePlan validationPlan{};

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

    if (!useLuaScene) {
        ConfigureRenderGraphPasses();
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
        if (validationTourService_) {
            validationPlan = validationTourService_->BeginFrame(aspect);
        }

        ViewState viewState = sceneScriptService_->GetViewState(aspect);
        if (validationPlan.active && validationPlan.overrideView) {
            viewState = validationPlan.viewState;
        }

        graphicsService_->RenderScene(renderCommands, viewState);
    }

    if (validationPlan.active && validationPlan.requestScreenshot) {
        graphicsService_->RequestScreenshot(validationPlan.screenshotPath);
    }

    if (!graphicsService_->EndFrame()) {
        if (logger_) {
            logger_->Warn("RenderCoordinatorService::RenderFrame: Swapchain out of date during EndFrame");
        }
        graphicsService_->RecreateSwapchain();
        return;
    }

    if (validationPlan.active && validationTourService_) {
        ValidationFrameResult validationResult = validationTourService_->EndFrame();
        if (validationResult.shouldAbort) {
            if (logger_) {
                logger_->Error("RenderCoordinatorService::RenderFrame: Validation failed - " + validationResult.message);
            }
            throw std::runtime_error("Validation tour failed: " + validationResult.message);
        }
    }

    if (logger_) {
        logger_->Trace("RenderCoordinatorService", "RenderFrame", "", "Exiting");
    }
}

}  // namespace sdl3cpp::services::impl
