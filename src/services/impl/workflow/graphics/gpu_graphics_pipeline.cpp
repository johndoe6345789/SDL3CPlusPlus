#include "services/interfaces/workflow/graphics/gpu_graphics_pipeline.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {
namespace {

std::string StrParam(const WorkflowStepParameterResolver& params,
                     const WorkflowStepDefinition& step, const char* key,
                     const std::string& fallback) {
    const auto* p       = params.FindParameter(step, key);
    const bool isString = p && p->type == WorkflowParameterValue::Type::String;
    return isString ? p->stringValue : fallback;
}

float NumParam(const WorkflowStepParameterResolver& params,
               const WorkflowStepDefinition& step, const char* key,
               float fallback) {
    const auto* p       = params.FindParameter(step, key);
    const bool isNumber = p && p->type == WorkflowParameterValue::Type::Number;
    return isNumber ? static_cast<float>(p->numberValue) : fallback;
}

bool BoolParam(const WorkflowStepParameterResolver& params,
               const WorkflowStepDefinition& step, const char* key,
               bool fallback) {
    return static_cast<int>(NumParam(params, step, key, fallback ? 1 : 0)) != 0;
}

}  // namespace

GpuPipelineCreateParams ReadGpuPipelineCreateParams(
    const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;
    GpuPipelineCreateParams p;

    p.vertexShaderKey =
        StrParam(params, step, "vertex_shader_key", p.vertexShaderKey);
    p.fragmentShaderKey =
        StrParam(params, step, "fragment_shader_key", p.fragmentShaderKey);
    p.vertexFormat = StrParam(params, step, "vertex_format", p.vertexFormat);
    p.pipelineKey  = StrParam(params, step, "pipeline_key", p.pipelineKey);
    p.depthWrite   = BoolParam(params, step, "depth_write", p.depthWrite);
    p.depthTest    = BoolParam(params, step, "depth_test", p.depthTest);
    p.cullMode     = StrParam(params, step, "cull_mode", p.cullMode);
    p.depthBias    = NumParam(params, step, "depth_bias", p.depthBias);
    p.depthBiasSlope =
        NumParam(params, step, "depth_bias_slope", p.depthBiasSlope);
    p.numColorTargets = static_cast<int>(
        NumParam(params, step, "num_color_targets", p.numColorTargets));
    p.depthFormat = StrParam(params, step, "depth_format", p.depthFormat);
    p.releaseShaders =
        BoolParam(params, step, "release_shaders", p.releaseShaders);
    p.colorFormat = StrParam(params, step, "color_format", p.colorFormat);
    p.hasDepth    = BoolParam(params, step, "has_depth", p.hasDepth);
    p.alphaBlend  = BoolParam(params, step, "alpha_blend", p.alphaBlend);
    return p;
}

GpuVertexAttributeLayout BuildVertexAttributeLayout(
    const std::string& vertexFormat) {
    GpuVertexAttributeLayout layout;
    auto& vbuf              = layout.vbufDesc;
    auto& attrs             = layout.attrs;
    vbuf.slot               = 0;
    vbuf.input_rate         = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vbuf.instance_step_rate = 0;
    attrs[0].location       = 0;
    attrs[0].buffer_slot    = 0;
    attrs[0].format         = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    attrs[0].offset         = 0;

    if (vertexFormat == "none") {
        // Fullscreen triangle: no vertex buffers, vertex_id only.
        layout.numBuffers    = 0;
        layout.numAttributes = 0;
    } else if (vertexFormat == "position_uv_lmuv_normal") {
        // BSP: float3 pos + float2 uv + float2 lmuv + float3 normal = 40B.
        vbuf.pitch           = sizeof(float) * 10;
        attrs[1].location    = 1;
        attrs[1].buffer_slot = 0;
        attrs[1].format      = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        attrs[1].offset      = sizeof(float) * 3;  // 12
        attrs[2].location    = 2;
        attrs[2].buffer_slot = 0;
        attrs[2].format      = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        attrs[2].offset      = sizeof(float) * 5;  // 20
        attrs[3].location    = 3;
        attrs[3].buffer_slot = 0;
        attrs[3].format      = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        attrs[3].offset      = sizeof(float) * 7;  // 28
        layout.numBuffers    = 1;
        layout.numAttributes = 4;
    } else if (vertexFormat == "position_uv") {
        // Textured: float3 position + float2 uv = 20 bytes.
        vbuf.pitch           = sizeof(float) * 5;
        attrs[1].location    = 1;
        attrs[1].buffer_slot = 0;
        attrs[1].format      = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        attrs[1].offset      = sizeof(float) * 3;
        layout.numBuffers    = 1;
        layout.numAttributes = 2;
    } else {
        // Default position_color: float3 position + ubyte4 color = 16B.
        vbuf.pitch           = sizeof(float) * 3 + sizeof(uint8_t) * 4;
        attrs[1].location    = 1;
        attrs[1].buffer_slot = 0;
        attrs[1].format      = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
        attrs[1].offset      = sizeof(float) * 3;
        layout.numBuffers    = 1;
        layout.numAttributes = 2;
    }
    return layout;
}

SDL_GPUCullMode ResolveCullMode(const std::string& cullMode) {
    if (cullMode == "front") {
        return SDL_GPU_CULLMODE_FRONT;
    }
    if (cullMode == "none") {
        return SDL_GPU_CULLMODE_NONE;
    }
    return SDL_GPU_CULLMODE_BACK;
}

SDL_GPUTextureFormat ResolveDepthFormat(const std::string& depthFormat) {
    if (depthFormat == "d24_unorm_s8") {
        return SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
    }
    return SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
}

SDL_GPUTextureFormat ResolveColorTargetFormat(const std::string& colorFormat,
                                              SDL_GPUDevice* device,
                                              SDL_Window* window) {
    if (colorFormat == "rgba16_float") {
        return SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
    }
    if (colorFormat == "r8_unorm") {
        return SDL_GPU_TEXTUREFORMAT_R8_UNORM;
    }
    if (colorFormat == "b8g8r8a8_unorm") {
        return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
    }
    // "swapchain" (default) — get format from window.
    if (window) {
        return SDL_GetGPUSwapchainTextureFormat(device, window);
    }
    return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
}

void ApplyAlphaBlendState(SDL_GPUColorTargetDescription& target) {
    target.blend_state.enable_blend          = true;
    target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    target.blend_state.dst_color_blendfactor =
        SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    target.blend_state.color_blend_op        = SDL_GPU_BLENDOP_ADD;
    target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    target.blend_state.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;
}

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

GpuPipelineShaders RequireGpuPipelineShaders(WorkflowContext& context,
                                             const GpuPipelineCreateParams& p) {
    GpuPipelineShaders shaders;
    shaders.vertex = context.Get<SDL_GPUShader*>(p.vertexShaderKey, nullptr);
    shaders.fragment =
        context.Get<SDL_GPUShader*>(p.fragmentShaderKey, nullptr);
    if (!shaders.vertex) {
        throw std::runtime_error(
            "graphics.gpu.pipeline.create: Vertex shader not found at "
            "key '" +
            p.vertexShaderKey + "'");
    }
    if (!shaders.fragment) {
        throw std::runtime_error(
            "graphics.gpu.pipeline.create: Fragment shader not found at "
            "key '" +
            p.fragmentShaderKey + "'");
    }
    return shaders;
}

}  // namespace sdl3cpp::services::impl
