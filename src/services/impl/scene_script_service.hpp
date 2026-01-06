#pragma once

#include "../interfaces/i_scene_script_service.hpp"
#include "../interfaces/i_script_engine_service.hpp"
#include "../interfaces/i_logger.hpp"
#include <memory>

struct lua_State;

namespace sdl3cpp::services::impl {

/**
 * @brief Script-facing scene service implementation.
 */
class SceneScriptService : public ISceneScriptService {
public:
    SceneScriptService(std::shared_ptr<IScriptEngineService> engineService,
                       std::shared_ptr<ILogger> logger);

    std::vector<SceneObject> LoadSceneObjects() override;
    std::array<float, 16> ComputeModelMatrix(int functionRef, float time) override;
    ViewState GetViewState(float aspect) override;

private:
    lua_State* GetLuaState() const;

    std::shared_ptr<IScriptEngineService> engineService_;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
