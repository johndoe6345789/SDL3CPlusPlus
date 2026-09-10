#include "services/interfaces/workflow/rendering/grid_draw_cubes.hpp"
#include "services/interfaces/workflow/rendering/grid_render_pass.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace sdl3cpp::services::impl {

namespace {

/// Pushes this cube's MVP uniform and issues its indexed draw call.
void DrawGridCube(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass,
                  const GridDrawConfig& cfg, const glm::mat4& viewProj,
                  uint32_t xx, uint32_t yy, float time) {
    const float rotX = time + (static_cast<float>(xx) * cfg.rotOffsetX);
    const float rotY = time + (static_cast<float>(yy) * cfg.rotOffsetY);

    glm::mat4 model = glm::translate(
        glm::mat4(1.0f),
        glm::vec3(cfg.startX + (static_cast<float>(xx) * cfg.spacing),
                  cfg.startY + (static_cast<float>(yy) * cfg.spacing), 0.0f));
    model = glm::rotate(model, rotX, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, rotY, glm::vec3(0.0f, 1.0f, 0.0f));

    struct UniformData {
        float mvp[16];
    } uniforms;
    memcpy(uniforms.mvp, glm::value_ptr(viewProj * model),
           sizeof(uniforms.mvp));
    SDL_PushGPUVertexUniformData(cmd, 0, &uniforms, sizeof(uniforms));
    SDL_DrawGPUIndexedPrimitives(pass, 36, 1, 0, 0, 0);
}

}  // namespace

uint32_t DrawGridCubes(const GridGpuResources& gpu, const GridDrawConfig& cfg,
                       const glm::mat4& view, const glm::mat4& proj,
                       float time) {
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(gpu.device);
    if (!cmd) return 0;

    SDL_GPURenderPass* pass = BeginGridRenderPass(cmd, gpu, cfg);
    if (!pass) return 0;

    SDL_BindGPUGraphicsPipeline(pass, gpu.pipeline);
    SDL_GPUBufferBinding vb = {};
    vb.buffer               = gpu.vertexBuffer;
    SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
    SDL_GPUBufferBinding ib = {};
    ib.buffer               = gpu.indexBuffer;
    SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    const glm::mat4 viewProj = proj * view;
    uint32_t drawCalls       = 0;
    for (uint32_t yy = 0; yy < cfg.gridHeight; ++yy) {
        for (uint32_t xx = 0; xx < cfg.gridWidth; ++xx) {
            DrawGridCube(cmd, pass, cfg, viewProj, xx, yy, time);
            ++drawCalls;
        }
    }

    SDL_EndGPURenderPass(pass);
    SDL_SubmitGPUCommandBuffer(cmd);
    return drawCalls;
}

}  // namespace sdl3cpp::services::impl
