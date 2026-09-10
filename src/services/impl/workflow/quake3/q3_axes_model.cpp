#include "services/interfaces/workflow/quake3/q3_axes.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace sdl3cpp::q3 {

glm::mat4 ModelBasis(const glm::vec3& forward, const glm::vec3& up,
                     AxisConvention convention) {
    const glm::vec3 right = glm::cross(forward, up);
    glm::mat4 basis(1.0f);
    basis[0] = glm::vec4(forward, 0.0f);
    if (convention == AxisConvention::Ioq3ZUp) {
        // Vertices are still Quake's: X forward, Y left, Z up. This is
        // ioq3 AnglesToAxis, whose axis[1] is -right.
        basis[1] = glm::vec4(-right, 0.0f);
        basis[2] = glm::vec4(up, 0.0f);
    } else {
        // The MD3 loader already rewrote the vertices as (x, z, -y), so
        // the model's local +Y is up and its local +Z is right. Putting
        // up on column 1 is what keeps a model standing rather than
        // laid on its side.
        basis[1] = glm::vec4(up, 0.0f);
        basis[2] = glm::vec4(right, 0.0f);
    }
    return basis;
}

glm::mat4 PlaceModelWithBasis(const glm::vec3& pos, const glm::vec3& forward,
                              const glm::vec3& up, AxisConvention convention) {
    return glm::translate(glm::mat4(1.0f), pos) *
           ModelBasis(forward, up, convention);
}

glm::mat4 PlaceModel(const glm::vec3& pos, float yaw,
                     AxisConvention convention) {
    return PlaceModelWithBasis(pos, YawForward(yaw, convention),
                               AxisUp(convention), convention);
}

}  // namespace sdl3cpp::q3
