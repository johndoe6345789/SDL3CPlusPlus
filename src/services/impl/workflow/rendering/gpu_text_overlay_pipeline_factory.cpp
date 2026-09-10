#include "services/interfaces/workflow/rendering/gpu_text_overlay_pipeline_factory.hpp"

namespace sdl3cpp::services::impl {

SDL_GPUGraphicsPipeline* CreateOverlayPipeline(SDL_GPUDevice* device,
                                               SDL_GPUTextureFormat format,
                                               SDL_GPUShader* vertex,
                                               SDL_GPUShader* fragment) {
    // Vertex layout: float3 position + float2 uv.
    SDL_GPUVertexBufferDescription vbuf = {};
    vbuf.slot                           = 0;
    vbuf.pitch                          = sizeof(float) * 5;
    vbuf.input_rate                     = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexAttribute attrs[2] = {};
    attrs[0] = {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0};
    attrs[1] = {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, sizeof(float) * 3};

    SDL_GPUVertexInputState vis    = {};
    vis.vertex_buffer_descriptions = &vbuf;
    vis.num_vertex_buffers         = 1;
    vis.vertex_attributes          = attrs;
    vis.num_vertex_attributes      = 2;

    SDL_GPUColorTargetDescription ctd     = {};
    ctd.format                            = format;
    ctd.blend_state.enable_blend          = true;
    ctd.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    ctd.blend_state.dst_color_blendfactor =
        SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    ctd.blend_state.color_blend_op        = SDL_GPU_BLENDOP_ADD;
    ctd.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    ctd.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    ctd.blend_state.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;

    SDL_GPUGraphicsPipelineCreateInfo pci = {};
    pci.vertex_shader                     = vertex;
    pci.fragment_shader                   = fragment;
    pci.vertex_input_state                = vis;
    pci.primitive_type                    = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pci.rasterizer_state.fill_mode        = SDL_GPU_FILLMODE_FILL;
    pci.rasterizer_state.cull_mode        = SDL_GPU_CULLMODE_NONE;
    pci.rasterizer_state.front_face       = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    pci.depth_stencil_state.enable_depth_test  = false;
    pci.depth_stencil_state.enable_depth_write = false;
    pci.target_info.num_color_targets          = 1;
    pci.target_info.color_target_descriptions  = &ctd;
    pci.target_info.has_depth_stencil_target   = false;

    return SDL_CreateGPUGraphicsPipeline(device, &pci);
}

}  // namespace sdl3cpp::services::impl
