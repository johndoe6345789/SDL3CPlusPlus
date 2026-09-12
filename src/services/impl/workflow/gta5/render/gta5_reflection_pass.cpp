#include "services/interfaces/workflow/gta5/render/gta5_reflection_step.hpp"

#include "services/interfaces/workflow/gta5/render/gta5_draw_context_build.hpp"
#include "services/interfaces/workflow/gta5/render/gta5_draw_instances.hpp"
#include "services/interfaces/workflow/gta5/render/gta5_sky_uniforms.hpp"
#include "services/interfaces/workflow_context.hpp"

namespace sdl3cpp::services::impl {
namespace {

SDL_GPURenderPass* Begin(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* colour,
                         SDL_GPUTexture* depth) {
    SDL_GPUColorTargetInfo target = {};
    target.texture = colour;
    target.load_op = SDL_GPU_LOADOP_CLEAR;
    target.store_op = SDL_GPU_STOREOP_STORE;
    target.cycle = true;  // last frame's water may still be reading it
    SDL_GPUDepthStencilTargetInfo z = {};
    z.texture = depth;
    z.clear_depth = 1.f;
    z.load_op = SDL_GPU_LOADOP_CLEAR;
    z.store_op = SDL_GPU_STOREOP_DONT_CARE;
    z.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    z.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    z.cycle = true;
    return SDL_BeginGPURenderPass(cmd, &target, 1, &z);
}

}  // namespace

int WorkflowGta5ReflectionDrawStep::DrawMirror(
    const WorkflowStepDefinition& step, WorkflowContext& context,
    SDL_GPUCommandBuffer* cmd, const glm::mat4& view, const glm::mat4& proj,
    const glm::vec3& eye) {
    auto* sky = context.Get<SDL_GPUGraphicsPipeline*>(
        "gpu_pipeline_gta5_sky", nullptr);
    auto* opaque = context.Get<SDL_GPUGraphicsPipeline*>(
        "gpu_pipeline_gta5_reflect", nullptr);
    SDL_GPURenderPass* pass =
        sky && opaque ? Begin(cmd, colour_, depth_) : nullptr;
    if (!pass) return 0;
    // The sky first, as seen from below the water, then what stands in it.
    Gta5SkyUniforms mirrored = BuildGta5SkyUniforms(step, context);
    mirrored.invViewProj = glm::inverse(proj * view);
    mirrored.cameraPos = glm::vec4(eye, 1.f);
    SDL_BindGPUGraphicsPipeline(pass, sky);
    SDL_PushGPUFragmentUniformData(cmd, 0, &mirrored, sizeof(mirrored));
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    Gta5DrawContext draw = BuildGta5DrawContext(step, context);
    draw.pass = pass, draw.cmd = cmd, draw.view = view, draw.proj = proj;
    draw.cameraPos = eye;
    draw.terrainPipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        "gpu_pipeline_gta5_reflect_terrain", nullptr);
    draw.emissivePipeline = opaque;  // plain in the water
    draw.blendPipeline = nullptr;    // glass and decals are not mirrored
    SDL_BindGPUGraphicsPipeline(pass, opaque);
    const int drawn = DrawGta5Instances(*state_, draw, nullptr, &batch_);
    SDL_EndGPURenderPass(pass);
    return drawn;
}

}  // namespace sdl3cpp::services::impl
