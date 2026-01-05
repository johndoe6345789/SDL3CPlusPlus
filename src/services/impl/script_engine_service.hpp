#pragma once

#include "../interfaces/i_script_engine_service.hpp"
#include "../interfaces/i_audio_command_service.hpp"
#include "../interfaces/i_mesh_service.hpp"
#include "../interfaces/i_physics_bridge_service.hpp"
#include "../interfaces/i_input_service.hpp"
#include "../interfaces/i_window_service.hpp"
#include "../interfaces/i_config_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../../di/lifecycle.hpp"
#include <filesystem>
#include <memory>

struct lua_State;

namespace sdl3cpp::services::impl {

/**
 * @brief Service that owns the Lua runtime and script bindings.
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
                        std::shared_ptr<IInputService> inputService,
                        std::shared_ptr<IWindowService> windowService,
                        std::shared_ptr<IConfigService> configService,
                        bool debugEnabled = false);
    ~ScriptEngineService() override;

    // Lifecycle
    void Initialize() override;
    void Shutdown() noexcept override;

    // IScriptEngineService interface
    lua_State* GetLuaState() const override;
    std::filesystem::path GetScriptDirectory() const override;
    bool IsInitialized() const override {
        if (logger_) {
            logger_->Trace("ScriptEngineService", "IsInitialized");
        }
        return initialized_;
    }

private:
    struct LuaBindingContext {
        std::shared_ptr<IMeshService> meshService;
        std::shared_ptr<IAudioCommandService> audioCommandService;
        std::shared_ptr<IPhysicsBridgeService> physicsBridgeService;
        std::shared_ptr<IInputService> inputService;
        std::shared_ptr<IWindowService> windowService;
        std::shared_ptr<IConfigService> configService;
        std::shared_ptr<ILogger> logger;
    };

    void RegisterBindings(lua_State* L);
    static int LoadMeshFromFile(lua_State* L);
    static int PhysicsCreateBox(lua_State* L);
    static int PhysicsStepSimulation(lua_State* L);
    static int PhysicsGetTransform(lua_State* L);
    static int AudioPlayBackground(lua_State* L);
    static int AudioPlaySound(lua_State* L);
    static int AudioStopBackground(lua_State* L);
    static int GlmMatrixFromTransform(lua_State* L);
    static int InputGetMousePosition(lua_State* L);
    static int InputGetMouseDelta(lua_State* L);
    static int InputGetMouseWheel(lua_State* L);
    static int InputIsKeyDown(lua_State* L);
    static int InputIsActionDown(lua_State* L);
    static int InputIsMouseDown(lua_State* L);
    static int InputGetText(lua_State* L);
    static int ConfigGetJson(lua_State* L);
    static int ConfigGetTable(lua_State* L);
    static int WindowGetSize(lua_State* L);
    static int WindowSetTitle(lua_State* L);
    static int WindowIsMinimized(lua_State* L);
    static int WindowSetMouseGrabbed(lua_State* L);
    static int WindowGetMouseGrabbed(lua_State* L);
    static int WindowSetRelativeMouseMode(lua_State* L);
    static int WindowGetRelativeMouseMode(lua_State* L);
    static int WindowSetCursorVisible(lua_State* L);
    static int WindowIsCursorVisible(lua_State* L);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IMeshService> meshService_;
    std::shared_ptr<IAudioCommandService> audioCommandService_;
    std::shared_ptr<IPhysicsBridgeService> physicsBridgeService_;
    std::shared_ptr<IInputService> inputService_;
    std::shared_ptr<IWindowService> windowService_;
    std::shared_ptr<IConfigService> configService_;
    std::filesystem::path scriptPath_;
    std::filesystem::path scriptDirectory_;
    bool debugEnabled_ = false;
    bool initialized_ = false;
    lua_State* luaState_ = nullptr;
    std::shared_ptr<LuaBindingContext> bindingContext_;
};

}  // namespace sdl3cpp::services::impl
