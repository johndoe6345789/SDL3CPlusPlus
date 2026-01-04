#pragma once

#include "../interfaces/i_script_engine_service.hpp"
#include "../interfaces/i_audio_command_service.hpp"
#include "../interfaces/i_mesh_service.hpp"
#include "../interfaces/i_physics_bridge_service.hpp"
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
                        bool debugEnabled = false);
    ~ScriptEngineService() override;

    // Lifecycle
    void Initialize() override;
    void Shutdown() noexcept override;

    // IScriptEngineService interface
    lua_State* GetLuaState() const override;
    std::filesystem::path GetScriptDirectory() const override;
    bool IsInitialized() const override { return initialized_; }

private:
    struct LuaBindingContext {
        std::shared_ptr<IMeshService> meshService;
        std::shared_ptr<IAudioCommandService> audioCommandService;
        std::shared_ptr<IPhysicsBridgeService> physicsBridgeService;
        std::shared_ptr<ILogger> logger;
    };

    void RegisterBindings(lua_State* L);
    static int LoadMeshFromFile(lua_State* L);
    static int PhysicsCreateBox(lua_State* L);
    static int PhysicsStepSimulation(lua_State* L);
    static int PhysicsGetTransform(lua_State* L);
    static int AudioPlayBackground(lua_State* L);
    static int AudioPlaySound(lua_State* L);
    static int GlmMatrixFromTransform(lua_State* L);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IMeshService> meshService_;
    std::shared_ptr<IAudioCommandService> audioCommandService_;
    std::shared_ptr<IPhysicsBridgeService> physicsBridgeService_;
    std::filesystem::path scriptPath_;
    std::filesystem::path scriptDirectory_;
    bool debugEnabled_ = false;
    bool initialized_ = false;
    lua_State* luaState_ = nullptr;
    std::shared_ptr<LuaBindingContext> bindingContext_;
};

}  // namespace sdl3cpp::services::impl
