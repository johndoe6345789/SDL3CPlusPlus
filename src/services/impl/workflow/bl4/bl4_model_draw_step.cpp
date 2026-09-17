#include "services/interfaces/workflow/bl4/bl4_model_draw_step.hpp"

#include "services/interfaces/workflow/bl4/bl4_model_uniforms.hpp"
#include "services/interfaces/workflow/bl4/bl4_step_params.hpp"

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

void DrawInstance(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                  const Bl4Instance& instance, Bl4ModelVertexUniforms vertex) {
    if (!instance.geometry || !instance.geometry->usable) return;
    vertex.model = instance.modelMatrix;
    SDL_PushGPUVertexUniformData(cmd, 0, &vertex, sizeof(vertex));
    for (const Bl4SubMesh& sub : instance.geometry->subMeshes) {
        if (sub.indexCount == 0) continue;
        SDL_GPUBufferBinding vb{};
        vb.buffer = sub.vertexBuffer;
        SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
        SDL_GPUBufferBinding ib{};
        ib.buffer = sub.indexBuffer;
        SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_32BIT);
        SDL_DrawGPUIndexedPrimitives(pass, sub.indexCount, 1, 0, 0, 0);
    }
}

}  // namespace

WorkflowBl4ModelDrawStep::WorkflowBl4ModelDrawStep(std::shared_ptr<ILogger> logger,
                                                  std::shared_ptr<Bl4TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowBl4ModelDrawStep::GetPluginId() const { return "bl4.models.draw"; }

void WorkflowBl4ModelDrawStep::Execute(const WorkflowStepDefinition& step,
                                       WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        Bl4StringOr(step, "pipeline_key", "gpu_pipeline_bl4_model"), nullptr);
    if (!pass || !cmd || !pipeline || state_->resident.empty()) {
        if (logger_ && !warned_ && pass && !pipeline) {
            logger_->Warn("bl4.models.draw: no pipeline; models not drawn");
            warned_ = true;
        }
        return;
    }

    const std::string textureKey = Bl4StringOr(step, "texture_key", "bl4_placeholder");
    SDL_GPUTextureSamplerBinding albedo{context.Get<SDL_GPUTexture*>(textureKey + "_gpu", nullptr),
                                       context.Get<SDL_GPUSampler*>(textureKey + "_sampler", nullptr)};
    if (!albedo.texture || !albedo.sampler) {
        if (logger_ && !warned_) {
            logger_->Warn("bl4.models.draw: no '" + textureKey + "' texture; models not drawn");
            warned_ = true;
        }
        return;
    }

    Bl4ModelVertexUniforms vertex;
    vertex.viewProj = BuildBl4ViewProj(context);
    const Bl4ModelFragmentUniforms fragment = BuildBl4ModelFragmentUniforms(context);

    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_BindGPUFragmentSamplers(pass, 0, &albedo, 1);
    SDL_PushGPUFragmentUniformData(cmd, 0, &fragment, sizeof(fragment));

    for (const auto& [key, tile] : state_->resident) {
        for (const Bl4Instance& instance : tile.instances) DrawInstance(pass, cmd, instance, vertex);
    }
}

}  // namespace sdl3cpp::services::impl
