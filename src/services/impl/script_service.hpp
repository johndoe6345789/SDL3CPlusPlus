#pragma once

#include "../interfaces/i_script_service.hpp"
#include "../../di/lifecycle.hpp"
#include <memory>
#include <filesystem>

namespace sdl3cpp::script {
class ScriptEngine;
}

namespace sdl3cpp::services::impl {

/**
 * @brief Script service implementation.
 *
 * Wraps ScriptEngine to provide Lua script execution and integration
 * with scene, shaders, GUI, physics, and audio systems.
 */
class ScriptService : public IScriptService,
                      public di::IInitializable,
                      public di::IShutdownable {
public:
    explicit ScriptService(const std::filesystem::path& scriptPath);
    ~ScriptService() override;

    // IInitializable interface
    void Initialize() override;

    // IShutdownable interface
    void Shutdown() noexcept override;

    // IScriptService interface
    std::vector<script::SceneManager::SceneObject> LoadSceneObjects() override;
    std::array<float, 16> ComputeModelMatrix(int functionRef, float time) override;
    std::array<float, 16> GetViewProjectionMatrix(float aspect) override;
    std::unordered_map<std::string, script::ShaderManager::ShaderPaths> LoadShaderPathsMap() override;
    std::vector<script::GuiCommand> LoadGuiCommands() override;
    void UpdateGuiInput(const script::GuiInputSnapshot& input) override;
    bool HasGuiCommands() const override;
    script::PhysicsBridge& GetPhysicsBridge() override;
    void SetAudioPlayer(app::AudioPlayer* audioPlayer) override;
    std::filesystem::path GetScriptDirectory() const override;
    std::string GetLuaError() override;

private:
    std::filesystem::path scriptPath_;
    std::unique_ptr<script::ScriptEngine> scriptEngine_;
    bool initialized_ = false;
};

}  // namespace sdl3cpp::services::impl