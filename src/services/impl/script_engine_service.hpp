#pragma once

#include "../interfaces/i_script_engine_service.hpp"
#include "../interfaces/i_audio_command_service.hpp"
#include "../interfaces/i_mesh_service.hpp"
#include "../interfaces/i_physics_bridge_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../../di/lifecycle.hpp"
#include <filesystem>
#include <memory>

namespace sdl3cpp::script {
struct LuaBindingContext;
}

namespace sdl3cpp::services::impl {

/**
 * @brief Service wrapper around ScriptEngine.
 */
class ScriptEngineService : public IScriptEngineService,
                            public di::IInitializable,
                            public di::IShutdownable {
public:
    ScriptEngineService(const std::filesystem::path& scriptPath,
                        std::shared_ptr<ILogger> logger,
                        std::shared_ptr<IMeshService> meshService,
                        std::shared_ptr<IAudioCommandService> audioCommandService,
                        std::shared_ptr<IPhysicsBridgeService> physicsBridgeService,
                        bool debugEnabled = false);
    ~ScriptEngineService() override;

    // Lifecycle
    void Initialize() override;
    void Shutdown() noexcept override;

    // IScriptEngineService interface
    script::ScriptEngine& GetEngine() override;
    std::filesystem::path GetScriptDirectory() const override;
    bool IsInitialized() const override { return initialized_; }

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IMeshService> meshService_;
    std::shared_ptr<IAudioCommandService> audioCommandService_;
    std::shared_ptr<IPhysicsBridgeService> physicsBridgeService_;
    std::filesystem::path scriptPath_;
    bool debugEnabled_ = false;
    bool initialized_ = false;
    std::unique_ptr<script::ScriptEngine> engine_;
    std::shared_ptr<script::LuaBindingContext> bindingContext_;
};

}  // namespace sdl3cpp::services::impl
