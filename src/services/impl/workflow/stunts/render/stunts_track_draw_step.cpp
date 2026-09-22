#include "services/interfaces/workflow/stunts/render/stunts_track_draw_step.hpp"

#include "services/interfaces/workflow/rendering/draw_map_vertex_uniforms.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow/stunts/stunts_step_params.hpp"

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

void DrawMesh(SDL_GPURenderPass* pass, const StuntsGpuMesh& mesh) {
    if (!mesh.vertexBuffer || !mesh.indexBuffer || mesh.indexCount == 0) {
        return;
    }
    SDL_GPUBufferBinding vertices = {};
    vertices.buffer = mesh.vertexBuffer;
    SDL_BindGPUVertexBuffers(pass, 0, &vertices, 1);
    SDL_GPUBufferBinding indices = {};
    indices.buffer = mesh.indexBuffer;
    SDL_BindGPUIndexBuffer(pass, &indices, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_DrawGPUIndexedPrimitives(pass, mesh.indexCount, 1, 0, 0, 0);
}

}  // namespace

WorkflowStuntsTrackDrawStep::WorkflowStuntsTrackDrawStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<StuntsWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowStuntsTrackDrawStep::GetPluginId() const {
    return "stunts.track.draw";
}

void WorkflowStuntsTrackDrawStep::Execute(const WorkflowStepDefinition& step,
                                          WorkflowContext& context) {
    if (context.GetBool("frame_skip", false) || !state_->loaded) return;
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline =
        context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_textured", nullptr);
    if (!pass || !cmd || !pipeline) return;

    const rendering::VertexUniformData vertex = BuildDrawMapVertexUniforms(
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f)),
        context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.f)),
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.f)),
        context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.f)));
    auto fragment = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});

    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_PushGPUVertexUniformData(cmd, 0, &vertex, sizeof(vertex));
    SDL_PushGPUFragmentUniformData(cmd, 0, &fragment, sizeof(fragment));

    if (!traced_ && logger_) {
        traced_ = true;
        logger_->Info("stunts.track.draw: first frame, ground " +
                      std::to_string(state_->ground.indexCount) +
                      " indices, road " +
                      std::to_string(state_->road.indexCount));
    }
    BindTexture(pass, context,
                StuntsStringOr(step, "ground_key", nullptr, "ground_texture"));
    DrawMesh(pass, state_->ground);
    BindTexture(pass, context,
                StuntsStringOr(step, "road_key", nullptr, "road_texture"));
    DrawMesh(pass, state_->road);
}

}  // namespace sdl3cpp::services::impl
