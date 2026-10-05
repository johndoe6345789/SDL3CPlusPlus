#include "services/interfaces/workflow/racer/render/racer_model_draw.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace sdl3cpp::services::impl {

int DrawRacerModel(RacerDrawPass& d, const RacerGpuModel& model,
                   const glm::mat4& m, bool blended) {
    d.vertex.model = m;
    SDL_PushGPUVertexUniformData(d.cmd, 0, &d.vertex, sizeof(d.vertex));
    int drawn = 0;
    for (const RacerGpuBatch& batch : model.batches) {
        if (batch.blended != blended || !batch.texture || !batch.sampler) {
            continue;
        }
        const SDL_GPUTextureSamplerBinding binding = {batch.texture,
                                                      batch.sampler};
        SDL_BindGPUFragmentSamplers(d.pass, 0, &binding, 1);
        SDL_GPUBufferBinding vertices = {batch.vertices, 0};
        SDL_BindGPUVertexBuffers(d.pass, 0, &vertices, 1);
        SDL_DrawGPUPrimitives(d.pass, batch.vertexCount, 1, 0, 0);
        ++drawn;
    }
    return drawn;
}

glm::mat4 RacerPodMatrix(const RacerWorldState& state) {
    const RacerPodState& pod = state.pod;
    glm::mat4 m = glm::translate(glm::mat4(1.f), pod.position);
    m = glm::rotate(m, -pod.heading, glm::vec3(0.f, 1.f, 0.f));
    return glm::rotate(m, -state.podRoll, glm::vec3(0.f, 0.f, 1.f));
}

}  // namespace sdl3cpp::services::impl
