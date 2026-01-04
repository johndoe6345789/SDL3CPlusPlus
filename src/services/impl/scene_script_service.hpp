#pragma once

#include "../interfaces/i_scene_script_service.hpp"
#include "../interfaces/i_script_engine_service.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Script-facing scene service implementation.
 */
class SceneScriptService : public ISceneScriptService {
public:
    explicit SceneScriptService(std::shared_ptr<IScriptEngineService> engineService);

    std::vector<script::SceneManager::SceneObject> LoadSceneObjects() override;
    std::array<float, 16> ComputeModelMatrix(int functionRef, float time) override;
    std::array<float, 16> GetViewProjectionMatrix(float aspect) override;

private:
    std::shared_ptr<IScriptEngineService> engineService_;
};

}  // namespace sdl3cpp::services::impl
