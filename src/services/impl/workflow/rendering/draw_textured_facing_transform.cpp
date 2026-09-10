#include "services/interfaces/workflow/rendering/draw_textured_facing_transform.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace sdl3cpp::services::impl {

DrawTexturedTransform BuildFacingTransform(const std::string& facing,
                                           const glm::vec3& pos) {
    DrawTexturedTransform result;
    result.model  = glm::mat4(1.0f);
    result.normal = glm::vec3(0.0f, 1.0f, 0.0f);

    if (facing == "up") {
        result.model  = glm::translate(glm::mat4(1.0f), pos);
        result.normal = glm::vec3(0, 1, 0);
    } else if (facing == "down") {
        result.model  = glm::translate(glm::mat4(1.0f), pos);
        result.model  = glm::rotate(result.model, glm::radians(180.0f),
                                    glm::vec3(1, 0, 0));
        result.normal = glm::vec3(0, -1, 0);
    } else if (facing == "north") {
        result.model  = glm::translate(glm::mat4(1.0f), pos);
        result.model  = glm::rotate(result.model, glm::radians(-90.0f),
                                    glm::vec3(1, 0, 0));
        result.normal = glm::vec3(0, 0, -1);
    } else if (facing == "south") {
        result.model  = glm::translate(glm::mat4(1.0f), pos);
        result.model  = glm::rotate(result.model, glm::radians(90.0f),
                                    glm::vec3(1, 0, 0));
        result.normal = glm::vec3(0, 0, 1);
    } else if (facing == "east") {
        glm::mat4 rot(1.0f);
        rot[0]        = glm::vec4(0, 0, 1, 0);
        rot[1]        = glm::vec4(-1, 0, 0, 0);
        rot[2]        = glm::vec4(0, 1, 0, 0);
        rot[3]        = glm::vec4(0, 0, 0, 1);
        result.model  = glm::translate(glm::mat4(1.0f), pos) * rot;
        result.normal = glm::vec3(1, 0, 0);
    } else if (facing == "west") {
        glm::mat4 rot(1.0f);
        rot[0]        = glm::vec4(0, 0, -1, 0);
        rot[1]        = glm::vec4(1, 0, 0, 0);
        rot[2]        = glm::vec4(0, 1, 0, 0);
        rot[3]        = glm::vec4(0, 0, 0, 1);
        result.model  = glm::translate(glm::mat4(1.0f), pos) * rot;
        result.normal = glm::vec3(-1, 0, 0);
    }
    return result;
}

}  // namespace sdl3cpp::services::impl
