#include "services/interfaces/workflow/rendering/draw_textured_free_transform.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace sdl3cpp::services::impl {

DrawTexturedTransform BuildFreeTransform(const DrawTexturedParams& params) {
    DrawTexturedTransform result;
    result.model  = glm::mat4(1.0f);
    result.normal = glm::vec3(0.0f, 1.0f, 0.0f);

    result.model = glm::translate(
        result.model, glm::vec3(params.posX, params.posY, params.posZ));
    if (params.rotX != 0.0f) {
        result.model = glm::rotate(result.model, glm::radians(params.rotX),
                                   glm::vec3(1, 0, 0));
    }
    if (params.rotY != 0.0f) {
        result.model = glm::rotate(result.model, glm::radians(params.rotY),
                                   glm::vec3(0, 1, 0));
    }
    if (params.rotZ != 0.0f) {
        result.model = glm::rotate(result.model, glm::radians(params.rotZ),
                                   glm::vec3(0, 0, 1));
    }
    if (params.scale != 1.0f) {
        result.model = glm::scale(result.model, glm::vec3(params.scale));
    }
    return result;
}

}  // namespace sdl3cpp::services::impl
