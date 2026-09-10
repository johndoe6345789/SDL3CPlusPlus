#include "services/interfaces/workflow/workflow_generic_steps/camera_matrix_builder.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE  // Vulkan/Metal clip space [0,1]
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace sdl3cpp::services::impl {

nlohmann::json BuildCameraStateJson(const CameraSetupParams& params) {
    // Compute view matrix using GLM
    glm::mat4 viewMatrix =
        glm::lookAt(glm::vec3(0.0f, 0.0f, -params.distance),  // Eye
                    glm::vec3(0.0f, 0.0f, 0.0f),              // Center
                    glm::vec3(0.0f, 1.0f, 0.0f));             // Up

    // Compute projection matrix (GLM_FORCE_DEPTH_ZERO_TO_ONE for
    // Vulkan/Metal clip space)
    glm::mat4 projMatrix =
        glm::perspective(glm::radians(params.fov), params.aspectRatio,
                         params.nearPlane, params.farPlane);

    nlohmann::json cameraState = nlohmann::json::object();

    // Store view matrix as 16 floats (column-major, GLM default)
    nlohmann::json viewArray = nlohmann::json::array();
    const float* viewPtr     = glm::value_ptr(viewMatrix);
    for (int i = 0; i < 16; ++i) {
        viewArray.push_back(viewPtr[i]);
    }
    cameraState["view"] = viewArray;

    // Store projection matrix as 16 floats
    nlohmann::json projArray = nlohmann::json::array();
    const float* projPtr     = glm::value_ptr(projMatrix);
    for (int i = 0; i < 16; ++i) {
        projArray.push_back(projPtr[i]);
    }
    cameraState["projection"] = projArray;

    cameraState["distance"]             = params.distance;
    cameraState["fov"]                  = params.fov;
    cameraState["aspect_ratio"]         = params.aspectRatio;
    cameraState["near_plane"]           = params.nearPlane;
    cameraState["far_plane"]            = params.farPlane;
    cameraState["camera_setup_success"] = true;
    return cameraState;
}

}  // namespace sdl3cpp::services::impl
