#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_draw_step.hpp"

#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow/rendering/draw_map_vertex_uniforms.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

void BindTexture(SDL_GPURenderPass* pass, const WorkflowContext& context,
                 const std::string& key) {
    // texture.load publishes the device texture under "<key>_gpu".
    auto* texture = context.Get<SDL_GPUTexture*>(key + "_gpu", nullptr);
    auto* sampler = context.Get<SDL_GPUSampler*>(key + "_sampler", nullptr);
    if (!texture || !sampler) return;
    const SDL_GPUTextureSamplerBinding binding = {texture, sampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
}

void DrawChunk(SDL_GPURenderPass* pass, const SwitchbackTerrainChunk& chunk) {
    if (!chunk.vertexBuffer || !chunk.indexBuffer) return;
    SDL_GPUBufferBinding vertices = {};
    vertices.buffer = chunk.vertexBuffer;
    SDL_BindGPUVertexBuffers(pass, 0, &vertices, 1);
    SDL_GPUBufferBinding indices = {};
    indices.buffer = chunk.indexBuffer;
    SDL_BindGPUIndexBuffer(pass, &indices, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_DrawGPUIndexedPrimitives(pass, chunk.indexCount, 1, 0, 0, 0);
}

}  // namespace

WorkflowSwitchbackTerrainDrawStep::WorkflowSwitchbackTerrainDrawStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<SwitchbackTerrainState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowSwitchbackTerrainDrawStep::GetPluginId() const {
    return "switchback.terrain.draw";
}

void WorkflowSwitchbackTerrainDrawStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!state_ || state_->chunks.empty()) return;
    if (context.GetBool("frame_skip", false)) return;
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        Gta5ParameterOr(step, "pipeline_key", "gpu_pipeline_textured"),
        nullptr);
    if (!pass || !cmd || !pipeline) return;

    const rendering::VertexUniformData vertex = BuildDrawMapVertexUniforms(
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f)),
        context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.f)),
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.f)),
        context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.f)));
    const auto fragment = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});

    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_PushGPUVertexUniformData(cmd, 0, &vertex, sizeof(vertex));
    SDL_PushGPUFragmentUniformData(cmd, 0, &fragment, sizeof(fragment));
    BindTexture(pass, context,
                Gta5ParameterOr(step, "texture_key", "switchback_ground"));
    for (const SwitchbackTerrainChunk& chunk : state_->chunks) {
        DrawChunk(pass, chunk);
    }
}

}  // namespace sdl3cpp::services::impl
