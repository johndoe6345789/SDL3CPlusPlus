#include "services/interfaces/workflow/rendering/postfx_taa_jitter.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

float Halton(int index, int base) {
    float f = 1.0f;
    float r = 0.0f;
    int i   = index;
    while (i > 0) {
        f /= static_cast<float>(base);
        r += f * (i % base);
        i /= base;
    }
    return r;
}

void ApplyTaaProjectionJitter(WorkflowContext& context, int frameIdx,
                              uint32_t width, uint32_t height) {
    auto projMatrix =
        context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    const float jitterX =
        (Halton(frameIdx % 16 + 1, 2) - 0.5f) / static_cast<float>(width);
    const float jitterY =
        (Halton(frameIdx % 16 + 1, 3) - 0.5f) / static_cast<float>(height);
    glm::mat4 jitteredProj = projMatrix;
    jitteredProj[2][0] += jitterX * 2.0f;
    jitteredProj[2][1] += jitterY * 2.0f;
    context.Set<glm::mat4>("render.proj_matrix", jitteredProj);
}

}  // namespace sdl3cpp::services::impl
