#include "services/interfaces/workflow/gta5/render/gta5_model_matrix.hpp"

#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace sdl3cpp::services::impl {

std::array<float, 16> BuildGta5ModelMatrix(const Gta5Placement& placement) {
    glm::mat4 model = glm::translate(glm::mat4(1.f), placement.position);
    model *= glm::mat4_cast(placement.rotation);
    model = glm::scale(model, placement.scale);

    std::array<float, 16> flat{};
    std::memcpy(flat.data(), glm::value_ptr(model), sizeof(flat));
    return flat;
}

}  // namespace sdl3cpp::services::impl
