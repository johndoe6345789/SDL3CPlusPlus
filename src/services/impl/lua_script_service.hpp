#pragma once

#include "../interfaces/i_script_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../../script/script_engine.hpp"
#include "../../di/lifecycle.hpp"
#include <memory>
#include <filesystem>

namespace sdl3cpp::services::impl {

/**
 * @brief Lua script service implementation.
 *
 * Small wrapper service (~100 lines) around ScriptEngine.
 * Provides Lua script execution and integration with game systems.
 */
class LuaScriptService : public IScriptService,
                         public di::IInitializable,
                         public di::IShutdownable {
public:
    explicit LuaScriptService(const std::filesystem::path& scriptPath, std::shared_ptr<ILogger> logger, bool debugEnabled = false);
    ~LuaScriptService() override;

    // Lifecycle
    void Initialize() override;
    void Shutdown() noexcept override;

    // IScriptService interface
    std::vector<script::SceneManager::SceneObject> LoadSceneObjects() override;
    std::array<float, 16> ComputeModelMatrix(int functionRef, float time) override;
    std::array<float, 16> GetViewProjectionMatrix(float aspect) override;

    std::unordered_map<std::string, sdl3cpp::services::ShaderPaths> LoadShaderPathsMap() override;

    std::vector<script::GuiCommand> LoadGuiCommands() override;
    void UpdateGuiInput(const script::GuiInputSnapshot& input) override;
    bool HasGuiCommands() const override;

    script::PhysicsBridge& GetPhysicsBridge() override;

    void SetAudioPlayer(app::AudioPlayer* audioPlayer) override;

    std::filesystem::path GetScriptDirectory() const override;
    std::string GetLuaError() override;

private:
    std::shared_ptr<ILogger> logger_;
    std::unique_ptr<script::ScriptEngine> engine_;
    std::filesystem::path scriptPath_;
    bool debugEnabled_ = false;
    bool initialized_ = false;
};

}  // namespace sdl3cpp::services::impl
