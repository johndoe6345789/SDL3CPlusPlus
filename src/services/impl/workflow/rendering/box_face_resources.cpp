#include "services/interfaces/workflow/rendering/box_face_resources.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

bool ResolveTexturedBoxResources(WorkflowContext& context, ILogger* logger,
                                 const DrawTexturedBoxParams& params,
                                 TexturedBoxDrawContext& out) {
    out.pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    out.cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    out.pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        "gpu_pipeline_textured", nullptr);
    if (!out.pass || !out.cmd || !out.pipeline) return false;

    // Unit plane buffers (1x1 plane on XZ, normal +Y)
    out.vb = context.Get<SDL_GPUBuffer*>("plane_unit_vb", nullptr);
    out.ib = context.Get<SDL_GPUBuffer*>("plane_unit_ib", nullptr);
    const auto* mesh_meta = context.TryGet<nlohmann::json>("plane_unit");
    if (!out.vb || !out.ib || !mesh_meta) {
        if (logger) {
            logger->Warn(
                "draw.textured_box: unit plane not found in "
                "context");
        }
        return false;
    }
    out.indexCount = (*mesh_meta)["index_count"];

    out.texture =
        context.Get<SDL_GPUTexture*>(params.texture + "_gpu", nullptr);
    out.sampler =
        context.Get<SDL_GPUSampler*>(params.texture + "_sampler", nullptr);
    if (!out.texture || !out.sampler) {
        if (logger) {
            logger->Warn("draw.textured_box: texture '" + params.texture +
                         "' not found");
        }
        return false;
    }

    return true;
}

}  // namespace sdl3cpp::services::impl
