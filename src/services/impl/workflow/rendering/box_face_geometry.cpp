#include "services/interfaces/workflow/rendering/box_face_geometry.hpp"

#include "services/interfaces/workflow/rendering/box_face_builder.hpp"
#include "services/interfaces/workflow/rendering/box_face_context.hpp"
#include "services/interfaces/workflow/rendering/box_face_draw.hpp"

namespace sdl3cpp::services::impl {

void DrawTexturedBox(WorkflowContext& context, ILogger* logger,
                     const DrawTexturedBoxParams& params) {
    TexturedBoxDrawContext draw;
    if (!ResolveTexturedBoxDraw(context, logger, params, draw)) return;

    const auto faces = BuildBoxFaces(params.size.x, params.size.y,
                                     params.size.z, params.uvDensity);

    SDL_BindGPUGraphicsPipeline(draw.pass, draw.pipeline);

    auto* shadow_tex =
        context.Get<SDL_GPUTexture*>("shadow_depth_texture", nullptr);
    auto* shadow_samp =
        context.Get<SDL_GPUSampler*>("shadow_depth_sampler", nullptr);
    BindBoxTextures(draw.pass, draw.texture, draw.sampler, shadow_tex,
                    shadow_samp);

    SDL_GPUBufferBinding vb_binding = {};
    vb_binding.buffer               = draw.vb;
    SDL_BindGPUVertexBuffers(draw.pass, 0, &vb_binding, 1);
    SDL_GPUBufferBinding ib_binding = {};
    ib_binding.buffer               = draw.ib;
    SDL_BindGPUIndexBuffer(draw.pass, &ib_binding,
                           SDL_GPU_INDEXELEMENTSIZE_16BIT);

    auto shadowVP = context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.0f));

    DrawBoxFaces(draw.pass, draw.cmd, faces, draw.center, draw.bodyRotation,
                 draw.view, draw.proj, draw.camPos, shadowVP, draw.fu,
                 draw.indexCount);
}

}  // namespace sdl3cpp::services::impl
