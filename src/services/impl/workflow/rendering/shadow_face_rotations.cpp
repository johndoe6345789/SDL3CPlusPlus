#include "services/interfaces/workflow/rendering/shadow_face_rotations.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace sdl3cpp::services::impl {

ShadowFaceRotations BuildShadowFaceRotations() {
    ShadowFaceRotations r;
    r.none = glm::mat4(1.0f);
    r.down =
        glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1, 0, 0));
    r.north =
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0));
    r.south =
        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0));

    r.east    = glm::mat4(1.0f);
    r.east[0] = glm::vec4(0, 0, 1, 0);
    r.east[1] = glm::vec4(1, 0, 0, 0);
    r.east[2] = glm::vec4(0, 1, 0, 0);
    r.east[3] = glm::vec4(0, 0, 0, 1);

    r.west    = glm::mat4(1.0f);
    r.west[0] = glm::vec4(0, 0, -1, 0);
    r.west[1] = glm::vec4(-1, 0, 0, 0);
    r.west[2] = glm::vec4(0, 1, 0, 0);
    r.west[3] = glm::vec4(0, 0, 0, 1);
    return r;
}

}  // namespace sdl3cpp::services::impl
