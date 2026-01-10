#pragma once

#include "services/interfaces/i_render_coordinator_service.hpp"
#include "services/interfaces/i_config_compiler_service.hpp"
#include "services/interfaces/i_graphics_service.hpp"
#include "services/interfaces/i_gui_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_scene_service.hpp"
#include "services/interfaces/i_shader_system_registry.hpp"
#include "services/interfaces/i_validation_tour_service.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

class RenderCoordinatorService : public IRenderCoordinatorService {
public:
    RenderCoordinatorService(std::shared_ptr<ILogger> logger,
                             std::shared_ptr<IConfigCompilerService> configCompilerService,
                             std::shared_ptr<IGraphicsService> graphicsService,
                             std::shared_ptr<IShaderSystemRegistry> shaderSystemRegistry,
                             std::shared_ptr<IGuiService> guiService,
                             std::shared_ptr<ISceneService> sceneService,
                             std::shared_ptr<IValidationTourService> validationTourService);
    ~RenderCoordinatorService() override = default;

    void RenderFrame(float time) override;
    void RenderFrameWithViewState(float time, const ViewState& viewState) override;
    void RenderFrameWithOverrides(float time,
                                  const ViewState* viewState,
                                  const std::vector<GuiCommand>* guiCommands) override;

private:

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IConfigCompilerService> configCompilerService_;
    std::shared_ptr<IGraphicsService> graphicsService_;
    std::shared_ptr<IShaderSystemRegistry> shaderSystemRegistry_;
    std::shared_ptr<IGuiService> guiService_;
    std::shared_ptr<ISceneService> sceneService_;
    std::shared_ptr<IValidationTourService> validationTourService_;
    size_t lastVertexCount_ = 0;
    size_t lastIndexCount_ = 0;
    bool shadersLoaded_ = false;
    bool geometryUploaded_ = false;

    void ConfigureRenderGraphPasses();
    void RenderFrameInternal(float time,
                             const ViewState* overrideView,
                             const std::vector<GuiCommand>* guiCommands);
};

}  // namespace sdl3cpp::services::impl
