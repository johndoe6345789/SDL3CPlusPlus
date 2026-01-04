#pragma once

#include "../../script/scene_manager.hpp"
#include <array>
#include <vector>

namespace sdl3cpp::services {

/**
 * @brief Script-facing scene service interface.
 */
class ISceneScriptService {
public:
    virtual ~ISceneScriptService() = default;

    virtual std::vector<script::SceneManager::SceneObject> LoadSceneObjects() = 0;
    virtual std::array<float, 16> ComputeModelMatrix(int functionRef, float time) = 0;
    virtual std::array<float, 16> GetViewProjectionMatrix(float aspect) = 0;
};

}  // namespace sdl3cpp::services
