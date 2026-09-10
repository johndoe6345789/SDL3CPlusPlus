#include "services/interfaces/workflow/graphics/gpu_pipeline_create_info.hpp"
#include "services/interfaces/workflow/graphics/gpu_pipeline_formats.hpp"

namespace sdl3cpp::services::impl {

SDL_GPUGraphicsPipelineCreateInfo BuildGraphicsPipelineCreateInfo(
    const GpuPipelineCreateParams& p, SDL_GPUShader* vertexShader,
    SDL_GPUShader* fragmentShader, SDL_GPUDevice* device, SDL_Window* window,
    GpuVertexAttributeLayout& layoutOut,
    SDL_GPUColorTargetDescription& colorTargetOut) {
    layoutOut = BuildVertexAttributeLayout(p.vertexFormat);
    SDL_GPUVertexInputState vertexInput = {};
    vertexInput.num_vertex_buffers      = layoutOut.numBuffers;
    vertexInput.num_vertex_attributes   = layoutOut.numAttributes;
    if (layoutOut.numBuffers > 0) {
        vertexInput.vertex_buffer_descriptions = &layoutOut.vbufDesc;
        vertexInput.vertex_attributes          = layoutOut.attrs.data();
    }

    colorTargetOut = {};
    if (p.numColorTargets > 0) {
        colorTargetOut.format =
            ResolveColorTargetFormat(p.colorFormat, device, window);
        if (p.alphaBlend) {
            ApplyAlphaBlendState(colorTargetOut);
        }
    }

    SDL_GPUGraphicsPipelineCreateInfo info = {};
    info.vertex_shader                     = vertexShader;
    info.fragment_shader                   = fragmentShader;
    info.vertex_input_state                = vertexInput;
    info.primitive_type                    = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    info.rasterizer_state.fill_mode  = SDL_GPU_FILLMODE_FILL;
    info.rasterizer_state.cull_mode  = ResolveCullMode(p.cullMode);
    info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    if (p.depthBias != 0.0f || p.depthBiasSlope != 0.0f) {
        info.rasterizer_state.enable_depth_bias          = true;
        info.rasterizer_state.depth_bias_constant_factor = p.depthBias;
        info.rasterizer_state.depth_bias_slope_factor    = p.depthBiasSlope;
    }

    info.depth_stencil_state.enable_depth_test  = p.depthTest;
    info.depth_stencil_state.enable_depth_write = p.depthWrite;
    info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;

    if (p.numColorTargets > 0) {
        info.target_info.color_target_descriptions = &colorTargetOut;
        info.target_info.num_color_targets         = p.numColorTargets;
    }
    if (p.hasDepth) {
        info.target_info.depth_stencil_format =
            ResolveDepthFormat(p.depthFormat);
        info.target_info.has_depth_stencil_target = true;
    }
    return info;
}

}  // namespace sdl3cpp::services::impl
