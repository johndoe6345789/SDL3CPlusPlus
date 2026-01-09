#pragma once

#include "../interfaces/i_render_coordinator_service.hpp"
#include "../interfaces/i_config_service.hpp"
#include "../interfaces/i_config_compiler_service.hpp"
#include "../interfaces/i_graphics_service.hpp"
#include "../interfaces/i_gui_script_service.hpp"
#include "../interfaces/i_gui_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../interfaces/i_scene_script_service.hpp"
#include "../interfaces/i_scene_service.hpp"
#include "../interfaces/i_shader_script_service.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

class RenderCoordinatorService : public IRenderCoordinatorService {
public:
    RenderCoordinatorService(std::shared_ptr<ILogger> logger,
                             std::shared_ptr<IConfigService> configService,
                             std::shared_ptr<IConfigCompilerService> configCompilerService,
                             std::shared_ptr<IGraphicsService> graphicsService,
                             std::shared_ptr<ISceneScriptService> sceneScriptService,
                             std::shared_ptr<IShaderScriptService> shaderScriptService,
                             std::shared_ptr<IGuiScriptService> guiScriptService,
                             std::shared_ptr<IGuiService> guiService,
                             std::shared_ptr<ISceneService> sceneService);
    ~RenderCoordinatorService() override = default;

    void RenderFrame(float time) override;

private:
    void ConfigureRenderGraphPasses();

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IConfigService> configService_;
    std::shared_ptr<IConfigCompilerService> configCompilerService_;
    std::shared_ptr<IGraphicsService> graphicsService_;
    std::shared_ptr<ISceneScriptService> sceneScriptService_;
    std::shared_ptr<IShaderScriptService> shaderScriptService_;
    std::shared_ptr<IGuiScriptService> guiScriptService_;
    std::shared_ptr<IGuiService> guiService_;
    std::shared_ptr<ISceneService> sceneService_;
    size_t lastVertexCount_ = 0;
    size_t lastIndexCount_ = 0;
    bool shadersLoaded_ = false;
    bool geometryUploaded_ = false;
    bool configFirstLogged_ = false;
};

}  // namespace sdl3cpp::services::impl
