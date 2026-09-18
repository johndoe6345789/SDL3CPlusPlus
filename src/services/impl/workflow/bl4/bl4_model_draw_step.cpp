#include "services/interfaces/workflow/bl4/bl4_model_draw_step.hpp"

#include "services/interfaces/workflow/bl4/bl4_frustum.hpp"
#include "services/interfaces/workflow/bl4/bl4_model_uniforms.hpp"
#include "services/interfaces/workflow/bl4/bl4_step_params.hpp"

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

/// What one frame's draws share.
struct Bl4DrawPass {
    SDL_GPURenderPass* pass = nullptr;
    SDL_GPUCommandBuffer* cmd = nullptr;
    SDL_GPUTextureSamplerBinding fallback{};
    SDL_GPUTexture* boundTexture = nullptr;
};

void BindAlbedo(Bl4DrawPass& draw, const Bl4SubMesh& sub) {
    const SDL_GPUTextureSamplerBinding albedo =
        sub.texture ? SDL_GPUTextureSamplerBinding{sub.texture, sub.sampler} : draw.fallback;
    if (albedo.texture == draw.boundTexture) return;  // items are texture-sorted
    SDL_BindGPUFragmentSamplers(draw.pass, 0, &albedo, 1);
    draw.boundTexture = albedo.texture;
}

void DrawItem(Bl4DrawPass& draw, const Bl4DrawItem& item) {
    const Bl4SubMesh& sub = *item.sub;
    BindAlbedo(draw, sub);
    SDL_GPUBufferBinding vb{};
    vb.buffer = sub.vertexBuffer;
    SDL_BindGPUVertexBuffers(draw.pass, 0, &vb, 1);
    SDL_GPUBufferBinding ib{};
    ib.buffer = sub.indexBuffer;
    SDL_BindGPUIndexBuffer(draw.pass, &ib, SDL_GPU_INDEXELEMENTSIZE_32BIT);
    // first_instance indexes the storage buffer: see bl4_model.vert.
    SDL_DrawGPUIndexedPrimitives(draw.pass, sub.indexCount, item.count, 0, 0, item.first);
}

glm::vec3 CameraPosition(const WorkflowContext& context) {
    const auto view = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f));
    return glm::vec3(glm::inverse(view)[3]);
}

void CullToBatch(const Bl4TileStreamState& state, const glm::mat4& viewProj,
                 const glm::vec3& camera, float sizeRatio, Bl4InstanceBatch& batch) {
    const Bl4Frustum frustum = MakeBl4Frustum(viewProj);
    batch.visible.clear();
    for (const auto& [key, tile] : state.resident) {
        for (const Bl4Instance& instance : tile.instances) {
            if (!instance.geometry || !instance.geometry->usable) continue;
            if (!Bl4InstanceVisible(frustum, instance, camera, sizeRatio)) continue;
            batch.visible.push_back(&instance);
        }
    }
    BuildBl4InstanceBatch(batch);
}

}  // namespace

WorkflowBl4ModelDrawStep::WorkflowBl4ModelDrawStep(std::shared_ptr<ILogger> logger,
                                                  std::shared_ptr<Bl4TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowBl4ModelDrawStep::GetPluginId() const { return "bl4.models.draw"; }

void WorkflowBl4ModelDrawStep::Execute(const WorkflowStepDefinition& step,
                                       WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;
    Bl4DrawPass draw;
    draw.pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    draw.cmd = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        Bl4StringOr(step, "pipeline_key", "gpu_pipeline_bl4_model"), nullptr);
    if (!draw.pass || !draw.cmd || !device || !pipeline || state_->resident.empty()) {
        if (logger_ && !warned_ && draw.pass && !pipeline) {
            logger_->Warn("bl4.models.draw: no pipeline; models not drawn");
            warned_ = true;
        }
        return;
    }

    const std::string textureKey = Bl4StringOr(step, "texture_key", "bl4_placeholder");
    draw.fallback = {context.Get<SDL_GPUTexture*>(textureKey + "_gpu", nullptr),
                     context.Get<SDL_GPUSampler*>(textureKey + "_sampler", nullptr)};
    if (!draw.fallback.texture || !draw.fallback.sampler) {
        if (logger_ && !warned_) {
            logger_->Warn("bl4.models.draw: no '" + textureKey + "' texture; models not drawn");
            warned_ = true;
        }
        return;
    }

    Bl4ModelVertexUniforms vertex;
    vertex.viewProj = BuildBl4ViewProj(context);
    const Bl4ModelFragmentUniforms fragment = BuildBl4ModelFragmentUniforms(context);
    Bl4InstanceBatch& batch = state_->batch;
    CullToBatch(*state_, vertex.viewProj, CameraPosition(context),
                Bl4NumberOr(step, "size_ratio", 0.004f), batch);
    // Its own command buffer, submitted before this frame's: the draws
    // below read what it writes.
    if (!UploadBl4InstanceBatch(device, batch)) return;

    SDL_BindGPUGraphicsPipeline(draw.pass, pipeline);
    SDL_BindGPUVertexStorageBuffers(draw.pass, 0, &batch.buffer, 1);
    SDL_PushGPUVertexUniformData(draw.cmd, 0, &vertex, sizeof(vertex));
    SDL_PushGPUFragmentUniformData(draw.cmd, 0, &fragment, sizeof(fragment));
    for (const Bl4DrawItem& item : batch.items) DrawItem(draw, item);
}

}  // namespace sdl3cpp::services::impl
