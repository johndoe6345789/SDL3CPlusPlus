#include "services/interfaces/workflow/gta5/gta5_draw_bind.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace sdl3cpp::services::impl {

void BindGta5BatchShared(const Gta5StreamState& state,
                         const Gta5DrawContext& draw) {
    SDL_BindGPUVertexStorageBuffers(draw.pass, 0, &state.batch.buffer, 1);
    // Once a frame: each group's offset travels as first_instance.
    Gta5InstancedUniforms vu = {};
    const glm::mat4 viewProj = draw.proj * draw.view;
    std::memcpy(vu.viewProj, glm::value_ptr(viewProj), sizeof(vu.viewProj));
    std::memcpy(vu.shadowVP, glm::value_ptr(draw.shadowVP),
                sizeof(vu.shadowVP));
    vu.cameraPos[0] = draw.cameraPos.x;
    vu.cameraPos[1] = draw.cameraPos.y;
    vu.cameraPos[2] = draw.cameraPos.z;
    SDL_PushGPUVertexUniformData(draw.cmd, 0, &vu, sizeof(vu));
}

void BindGta5ArenaBlock(const Gta5StreamState& state,
                        const Gta5DrawContext& draw, int block) {
    SDL_GPUBufferBinding vb = {state.arena.Vertices(block), 0};
    SDL_BindGPUVertexBuffers(draw.pass, 0, &vb, 1);
    SDL_GPUBufferBinding ib = {state.arena.Indices(block), 0};
    SDL_BindGPUIndexBuffer(draw.pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);
}

}  // namespace sdl3cpp::services::impl
