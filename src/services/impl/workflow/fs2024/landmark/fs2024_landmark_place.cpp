#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_place.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace sdl3cpp::services::impl {

glm::mat4 Fs2024LandmarkModel(const glm::vec3& at, float headingDegrees,
                              float scale) {
    // Turning about +y is anticlockwise seen from above, and a heading
    // turns clockwise.
    const float turn = glm::radians(180.f - headingDegrees);
    glm::mat4 model = glm::translate(glm::mat4(1.f), at);
    model = glm::rotate(model, turn, glm::vec3(0.f, 1.f, 0.f));
    return glm::scale(model, glm::vec3(scale));
}

std::size_t ChooseFs2024LandmarkLod(
    const std::vector<sdl3cpp::fs2024::GltfLodInfo>& lods,
    std::size_t budgetBytes) {
    for (std::size_t i = 0; i < lods.size(); ++i) {
        if (lods[i].bytes <= budgetBytes) return i;
    }
    return lods.empty() ? 0 : lods.size() - 1;
}

}  // namespace sdl3cpp::services::impl
