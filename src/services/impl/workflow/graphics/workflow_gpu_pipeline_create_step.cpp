#include "services/interfaces/workflow/graphics/workflow_gpu_pipeline_create_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowGpuPipelineCreateStep::WorkflowGpuPipelineCreateStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGpuPipelineCreateStep::GetPluginId() const {
    return "graphics.gpu.pipeline.create";
}

void WorkflowGpuPipelineCreateStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepParameterResolver params;

    // Read parameters with defaults
    auto getStr = [&](const char* name, const std::string& def) -> std::string {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::String) ? p->stringValue : def;
    };
    auto getNum = [&](const char* name, float def) -> float {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number) ? static_cast<float>(p->numberValue) : def;
    };

    const std::string vertex_shader_key = getStr("vertex_shader_key", "vertex_shader");
    const std::string fragment_shader_key = getStr("fragment_shader_key", "fragment_shader");
    const std::string vertex_format = getStr("vertex_format", "position_color");
    const std::string pipeline_key = getStr("pipeline_key", "gpu_pipeline");
    const bool depth_write = static_cast<int>(getNum("depth_write", 1)) != 0;
    const bool depth_test = static_cast<int>(getNum("depth_test", 1)) != 0;
    const std::string cull_mode_str = getStr("cull_mode", "back");
    const float depth_bias = getNum("depth_bias", 0.0f);
    const float depth_bias_slope = getNum("depth_bias_slope", 0.0f);
    const int num_color_targets = static_cast<int>(getNum("num_color_targets", 1));
    const std::string depth_format_str = getStr("depth_format", "d32_float");
    const bool release_shaders = static_cast<int>(getNum("release_shaders", 1)) != 0;
    const std::string color_format_str = getStr("color_format", "swapchain");
    const bool has_depth = static_cast<int>(getNum("has_depth", 1)) != 0;
    const bool alpha_blend = static_cast<int>(getNum("alpha_blend", 0)) != 0;

    // Get GPU device
    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error("graphics.gpu.pipeline.create: GPU device not found in context");
    }

    // Get pre-compiled shaders from context
    SDL_GPUShader* vertex_shader = context.Get<SDL_GPUShader*>(vertex_shader_key, nullptr);
    SDL_GPUShader* fragment_shader = context.Get<SDL_GPUShader*>(fragment_shader_key, nullptr);

    if (!vertex_shader) {
        throw std::runtime_error("graphics.gpu.pipeline.create: Vertex shader not found at key '" +
                                 vertex_shader_key + "'");
    }
    if (!fragment_shader) {
        throw std::runtime_error("graphics.gpu.pipeline.create: Fragment shader not found at key '" +
                                 fragment_shader_key + "'");
    }

    // Build vertex layout based on format
    SDL_GPUVertexBufferDescription vbuf_desc = {};
    vbuf_desc.slot = 0;
    vbuf_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vbuf_desc.instance_step_rate = 0;

    SDL_GPUVertexAttribute attrs[4] = {};
    attrs[0].location = 0;
    attrs[0].buffer_slot = 0;
    attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    attrs[0].offset = 0;

    SDL_GPUVertexInputState vertex_input = {};

    if (vertex_format == "none") {
        // Fullscreen triangle: no vertex buffers, vertex_id only
        vertex_input.num_vertex_buffers = 0;
        vertex_input.num_vertex_attributes = 0;
    } else if (vertex_format == "position_uv_lmuv_normal") {
        // BSP: float3 pos + float2 uv + float2 lmuv + float3 normal = 40 bytes
        vbuf_desc.pitch = sizeof(float) * 10;
        attrs[1].location = 1;
        attrs[1].buffer_slot = 0;
        attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        attrs[1].offset = sizeof(float) * 3;   // 12
        attrs[2].location = 2;
        attrs[2].buffer_slot = 0;
        attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        attrs[2].offset = sizeof(float) * 5;   // 20
        attrs[3].location = 3;
        attrs[3].buffer_slot = 0;
        attrs[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        attrs[3].offset = sizeof(float) * 7;   // 28
        vertex_input.vertex_buffer_descriptions = &vbuf_desc;
        vertex_input.num_vertex_buffers = 1;
        vertex_input.vertex_attributes = attrs;
        vertex_input.num_vertex_attributes = 4;
    } else if (vertex_format == "position_uv") {
        // Textured: float3 position + float2 uv = 20 bytes
        vbuf_desc.pitch = sizeof(float) * 5;
        attrs[1].location = 1;
        attrs[1].buffer_slot = 0;
        attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        attrs[1].offset = sizeof(float) * 3;
        vertex_input.vertex_buffer_descriptions = &vbuf_desc;
        vertex_input.num_vertex_buffers = 1;
        vertex_input.vertex_attributes = attrs;
        vertex_input.num_vertex_attributes = 2;
    } else {
        // Default position_color: float3 position + ubyte4 color = 16 bytes
        vbuf_desc.pitch = sizeof(float) * 3 + sizeof(uint8_t) * 4;
        attrs[1].location = 1;
        attrs[1].buffer_slot = 0;
        attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
        attrs[1].offset = sizeof(float) * 3;
        vertex_input.vertex_buffer_descriptions = &vbuf_desc;
        vertex_input.num_vertex_buffers = 1;
        vertex_input.vertex_attributes = attrs;
        vertex_input.num_vertex_attributes = 2;
    }

    // Resolve cull mode
    SDL_GPUCullMode cull_mode = SDL_GPU_CULLMODE_BACK;
    if (cull_mode_str == "front") {
        cull_mode = SDL_GPU_CULLMODE_FRONT;
    } else if (cull_mode_str == "none") {
        cull_mode = SDL_GPU_CULLMODE_NONE;
    }

    // Resolve depth format
    SDL_GPUTextureFormat depth_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    if (depth_format_str == "d24_unorm_s8") {
        depth_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
    }

    // Build color target description (only if we have color targets)
    SDL_GPUColorTargetDescription color_target = {};
    if (num_color_targets > 0) {
        if (color_format_str == "rgba16_float") {
            color_target.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
        } else if (color_format_str == "r8_unorm") {
            color_target.format = SDL_GPU_TEXTUREFORMAT_R8_UNORM;
        } else if (color_format_str == "b8g8r8a8_unorm") {
            color_target.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
        } else {
            // "swapchain" (default) — get format from window
            SDL_Window* window = context.Get<SDL_Window*>("sdl_window", nullptr);
            if (window) {
                color_target.format = SDL_GetGPUSwapchainTextureFormat(device, window);
            } else {
                color_target.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
            }
        }

        if (alpha_blend) {
            color_target.blend_state.enable_blend = true;
            color_target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
            color_target.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            color_target.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
            color_target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
            color_target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
            color_target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        }
    }

    // Build pipeline create info
    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.vertex_shader = vertex_shader;
    pipeline_info.fragment_shader = fragment_shader;
    pipeline_info.vertex_input_state = vertex_input;
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    // Rasterizer state
    pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipeline_info.rasterizer_state.cull_mode = cull_mode;
    pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

    if (depth_bias != 0.0f || depth_bias_slope != 0.0f) {
        pipeline_info.rasterizer_state.enable_depth_bias = true;
        pipeline_info.rasterizer_state.depth_bias_constant_factor = depth_bias;
        pipeline_info.rasterizer_state.depth_bias_slope_factor = depth_bias_slope;
    }

    // Depth/stencil state
    pipeline_info.depth_stencil_state.enable_depth_test = depth_test;
    pipeline_info.depth_stencil_state.enable_depth_write = depth_write;
    pipeline_info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;

    // Target info
    if (num_color_targets > 0) {
        pipeline_info.target_info.color_target_descriptions = &color_target;
        pipeline_info.target_info.num_color_targets = num_color_targets;
    } else {
        pipeline_info.target_info.num_color_targets = 0;
    }
    if (has_depth) {
        pipeline_info.target_info.depth_stencil_format = depth_format;
        pipeline_info.target_info.has_depth_stencil_target = true;
    }

    // Create the pipeline
    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);

    // Optionally release shaders (they're baked into the pipeline now)
    if (release_shaders) {
        SDL_ReleaseGPUShader(device, vertex_shader);
        SDL_ReleaseGPUShader(device, fragment_shader);
        // Remove from context so nobody uses stale pointers
        context.Remove(vertex_shader_key);
        context.Remove(fragment_shader_key);
    }

    if (!pipeline) {
        throw std::runtime_error("graphics.gpu.pipeline.create: Failed to create graphics pipeline: " +
                                 std::string(SDL_GetError()));
    }

    // Store pipeline in context
    context.Set<SDL_GPUGraphicsPipeline*>(pipeline_key, pipeline);

    if (logger_) {
        logger_->Trace("WorkflowGpuPipelineCreateStep", "Execute",
                       "pipeline_key=" + pipeline_key + ", format=" + vertex_format +
                       ", cull=" + cull_mode_str + ", color_targets=" + std::to_string(num_color_targets) +
                       ", depth_bias=" + std::to_string(depth_bias),
                       "Graphics pipeline created");
    }
}

}  // namespace sdl3cpp::services::impl
