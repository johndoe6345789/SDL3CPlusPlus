#include "services/interfaces/workflow/rendering/viewmodel_transform.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace sdl3cpp::services::rendering {

CameraBasis ExtractCameraBasis(const glm::mat4& view) {
    CameraBasis basis{};
    basis.right = glm::vec3(view[0][0], view[1][0], view[2][0]);
    basis.up = glm::vec3(view[0][1], view[1][1], view[2][1]);
    basis.forward = -glm::vec3(view[0][2], view[1][2], view[2][2]);
    return basis;
}

glm::mat4 BuildViewmodelMatrix(const glm::mat4& view,
                               const glm::vec3& cameraPosition,
                               const glm::vec3& offset,
                               const glm::vec3& rotationDegrees,
                               float scale) {
    const CameraBasis basis = ExtractCameraBasis(view);

    const glm::vec3 position = cameraPosition +
                               basis.right * offset.x +
                               basis.up * offset.y +
                               basis.forward * (-offset.z);

    glm::mat4 localRotation(1.0f);
    if (rotationDegrees.x != 0.0f) {
        localRotation = glm::rotate(localRotation,
                                    glm::radians(rotationDegrees.x),
                                    glm::vec3(1, 0, 0));
    }
    if (rotationDegrees.y != 0.0f) {
        localRotation = glm::rotate(localRotation,
                                    glm::radians(rotationDegrees.y),
                                    glm::vec3(0, 1, 0));
    }
    if (rotationDegrees.z != 0.0f) {
        localRotation = glm::rotate(localRotation,
                                    glm::radians(rotationDegrees.z),
                                    glm::vec3(0, 0, 1));
    }

    glm::mat4 orientation(1.0f);
    orientation[0] = glm::vec4(basis.right, 0.0f);
    orientation[1] = glm::vec4(basis.up, 0.0f);
    orientation[2] = glm::vec4(-basis.forward, 0.0f);

    return glm::translate(glm::mat4(1.0f), position) * orientation *
           localRotation * glm::scale(glm::mat4(1.0f), glm::vec3(scale));
}

}  // namespace sdl3cpp::services::rendering
