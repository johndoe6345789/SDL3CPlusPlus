#include "services/interfaces/workflow/rendering/draw_textured_transform.hpp"
#include "services/interfaces/workflow/rendering/draw_textured_facing_transform.hpp"
#include "services/interfaces/workflow/rendering/draw_textured_free_transform.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace sdl3cpp::services::impl {

DrawTexturedTransform BuildDrawTexturedTransform(
    const DrawTexturedParams& params) {
    if (!params.facing.empty()) {
        const glm::vec3 pos(params.posX, params.posY, params.posZ);
        DrawTexturedTransform result = BuildFacingTransform(params.facing, pos);
        if (params.scale != 1.0f) {
            result.model = glm::scale(result.model, glm::vec3(params.scale));
        }
        return result;
    }
    return BuildFreeTransform(params);
}

}  // namespace sdl3cpp::services::impl
