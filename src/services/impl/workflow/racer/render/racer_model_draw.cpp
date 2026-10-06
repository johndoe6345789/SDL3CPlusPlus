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

int DrawRacerSky(RacerDrawPass& d, const RacerWorldState& state,
                 SDL_GPUGraphicsPipeline* blend, const glm::vec3& eye) {
    if (!blend || state.skyModel.batches.empty()) return 0;
    SDL_BindGPUGraphicsPipeline(d.pass, blend);
    // No fog on the sky: its fog distances are pushed out of reach.
    const RacerFragmentUniforms sky{{0.f, 0.f, 0.f, 1e6f},
                                    {2e6f, 0.5f, 0.f, 0.f},
                                    {eye, 1.f}};
    SDL_PushGPUFragmentUniformData(d.cmd, 0, &sky, sizeof(sky));
    const glm::mat4 at = glm::translate(glm::mat4(1.f), eye);
    int drawn = DrawRacerModel(d, state.skyModel, at, false);
    return drawn + DrawRacerModel(d, state.skyModel, at, true);
}

glm::mat4 RacerPodMatrix(const RacerPodState& pod, float roll) {
    glm::mat4 m = glm::translate(glm::mat4(1.f), pod.position);
    m = glm::rotate(m, -pod.heading, glm::vec3(0.f, 1.f, 0.f));
    m = glm::rotate(m, -roll, glm::vec3(0.f, 0.f, 1.f));
    // The pod's own space has its engines toward -z once converted; the
    // turn the game applies above the LOD node is not decoded, so the
    // model is turned here so the engines lead.
    return glm::rotate(m, 3.14159265f, glm::vec3(0.f, 1.f, 0.f));
}

}  // namespace sdl3cpp::services::impl
