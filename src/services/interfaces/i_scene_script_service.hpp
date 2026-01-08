#pragma once

#include "scene_types.hpp"
#include "graphics_types.hpp"
#include <vector>

namespace sdl3cpp::services {

/**
 * @brief Script-facing scene service interface.
 */
class ISceneScriptService {
public:
    virtual ~ISceneScriptService() = default;

    virtual std::vector<SceneObject> LoadSceneObjects() = 0;
    virtual std::array<float, 16> ComputeModelMatrix(int functionRef, float time) = 0;
    virtual ViewState GetViewState(float aspect) = 0;
};

}  // namespace sdl3cpp::services
