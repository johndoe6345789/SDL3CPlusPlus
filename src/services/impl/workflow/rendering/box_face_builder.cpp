#include "services/interfaces/workflow/rendering/box_face_builder.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace sdl3cpp::services::impl {

std::array<BoxFace, 6> BuildBoxFaces(float sizeX, float sizeY, float sizeZ,
                                     float uvDensity) {
    const float hx = sizeX * 0.5f;
    const float hy = sizeY * 0.5f;
    const float hz = sizeZ * 0.5f;

    const glm::mat4 rotNone(1.0f);
    const glm::mat4 rotDown =
        glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1, 0, 0));
    const glm::mat4 rotNorth =
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0));
    const glm::mat4 rotSouth =
        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0));

    glm::mat4 rotEast(1.0f);
    rotEast[0] = glm::vec4(0, 0, 1, 0);
    rotEast[1] = glm::vec4(1, 0, 0, 0);
    rotEast[2] = glm::vec4(0, 1, 0, 0);
    rotEast[3] = glm::vec4(0, 0, 0, 1);

    glm::mat4 rotWest(1.0f);
    rotWest[0] = glm::vec4(0, 0, -1, 0);
    rotWest[1] = glm::vec4(-1, 0, 0, 0);
    rotWest[2] = glm::vec4(0, 1, 0, 0);
    rotWest[3] = glm::vec4(0, 0, 0, 1);

    return {{
        {glm::vec3(0, hy, 0), glm::vec3(0, 1, 0), rotNone, sizeX, sizeZ,
         sizeX * uvDensity, sizeZ * uvDensity},
        {glm::vec3(0, -hy, 0), glm::vec3(0, -1, 0), rotDown, sizeX, sizeZ,
         sizeX * uvDensity, sizeZ * uvDensity},
        {glm::vec3(0, 0, -hz), glm::vec3(0, 0, -1), rotNorth, sizeX, sizeY,
         sizeX * uvDensity, sizeY * uvDensity},
        {glm::vec3(0, 0, hz), glm::vec3(0, 0, 1), rotSouth, sizeX, sizeY,
         sizeX * uvDensity, sizeY * uvDensity},
        {glm::vec3(hx, 0, 0), glm::vec3(1, 0, 0), rotEast, sizeZ, sizeY,
         sizeZ * uvDensity, sizeY * uvDensity},
        {glm::vec3(-hx, 0, 0), glm::vec3(-1, 0, 0), rotWest, sizeZ, sizeY,
         sizeZ * uvDensity, sizeY * uvDensity},
    }};
}

}  // namespace sdl3cpp::services::impl
