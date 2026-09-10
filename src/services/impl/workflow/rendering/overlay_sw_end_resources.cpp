#include "services/interfaces/workflow/rendering/overlay_sw_end_resources.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <fstream>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

std::vector<uint8_t> LoadBinary(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return {};
    }
    const auto size = file.tellg();
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

SDL_GPUGraphicsPipeline* CreatePipeline(SDL_GPUDevice* device,
                                        SDL_Window* window,
                                        SDL_GPUShader* vertex,
                                        SDL_GPUShader* fragment) {
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

    SDL_GPUColorTargetDescription ctd = {};
    ctd.format = window ? SDL_GetGPUSwapchainTextureFormat(device, window)
                        : SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
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

    return SDL_CreateGPUGraphicsPipeline(device, &pci);
}

}  // namespace

bool CreateOverlaySwEndResources(SDL_GPUDevice* device, SDL_Window* window,
                                 int surfaceWidth, int surfaceHeight,
                                 const std::string& vertPath,
                                 const std::string& fragPath,
                                 OverlaySwEndResources& out) {
    const char* driver           = SDL_GetGPUDeviceDriver(device);
    const std::string driverName = driver ? driver : "";
    const char* entry            = (driverName == "metal") ? "main0" : "main";

    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    if (driverName == "metal") {
        format = SDL_GPU_SHADERFORMAT_MSL;
    } else if (driverName == "vulkan") {
        format = SDL_GPU_SHADERFORMAT_SPIRV;
    } else {
        return false;
    }

    const std::vector<uint8_t> vertCode = LoadBinary(vertPath);
    const std::vector<uint8_t> fragCode = LoadBinary(fragPath);
    if (vertCode.empty() || fragCode.empty()) {
        return false;
    }

    SDL_GPUShaderCreateInfo vsi = {};
    vsi.code                    = vertCode.data();
    vsi.code_size               = vertCode.size();
    vsi.entrypoint              = entry;
    vsi.format                  = format;
    vsi.stage                   = SDL_GPU_SHADERSTAGE_VERTEX;

    SDL_GPUShaderCreateInfo fsi = {};
    fsi.code                    = fragCode.data();
    fsi.code_size               = fragCode.size();
    fsi.entrypoint              = entry;
    fsi.format                  = format;
    fsi.stage                   = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fsi.num_samplers            = 1;

    SDL_GPUShader* vertex   = SDL_CreateGPUShader(device, &vsi);
    SDL_GPUShader* fragment = SDL_CreateGPUShader(device, &fsi);
    if (!vertex || !fragment) {
        if (vertex) SDL_ReleaseGPUShader(device, vertex);
        if (fragment) SDL_ReleaseGPUShader(device, fragment);
        return false;
    }

    OverlaySwEndResources res;
    res.device   = device;
    res.pipeline = CreatePipeline(device, window, vertex, fragment);
    SDL_ReleaseGPUShader(device, vertex);
    SDL_ReleaseGPUShader(device, fragment);
    if (!res.pipeline) {
        return false;
    }

    SDL_GPUTextureCreateInfo tci = {};
    tci.type                     = SDL_GPU_TEXTURETYPE_2D;
    tci.format                   = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tci.width                    = static_cast<uint32_t>(surfaceWidth);
    tci.height                   = static_cast<uint32_t>(surfaceHeight);
    tci.layer_count_or_depth     = 1;
    tci.num_levels               = 1;
    tci.usage                    = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    res.texture                  = SDL_CreateGPUTexture(device, &tci);

    SDL_GPUTransferBufferCreateInfo tbci = {};
    tbci.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size    = static_cast<uint32_t>(surfaceWidth * surfaceHeight * 4);
    res.transfer = SDL_CreateGPUTransferBuffer(device, &tbci);

    SDL_GPUBufferCreateInfo bci = {};
    bci.usage                   = SDL_GPU_BUFFERUSAGE_VERTEX;
    bci.size     = 6u * 5u * static_cast<uint32_t>(sizeof(float));
    res.vertices = SDL_CreateGPUBuffer(device, &bci);

    SDL_GPUSamplerCreateInfo sci = {};
    sci.min_filter               = SDL_GPU_FILTER_NEAREST;
    sci.mag_filter               = SDL_GPU_FILTER_NEAREST;
    sci.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sci.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sci.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    res.sampler                  = SDL_CreateGPUSampler(device, &sci);

    if (!res.texture || !res.transfer || !res.vertices || !res.sampler) {
        DestroyOverlaySwEndResources(res);
        return false;
    }

    out = res;
    return true;
}

void DestroyOverlaySwEndResources(OverlaySwEndResources& res) {
    if (res.device) {
        if (res.headSampler) SDL_ReleaseGPUSampler(res.device, res.headSampler);
        if (res.headVertices)
            SDL_ReleaseGPUBuffer(res.device, res.headVertices);
        if (res.sampler) SDL_ReleaseGPUSampler(res.device, res.sampler);
        if (res.vertices) SDL_ReleaseGPUBuffer(res.device, res.vertices);
        if (res.transfer) {
            SDL_ReleaseGPUTransferBuffer(res.device, res.transfer);
        }
        if (res.texture) SDL_ReleaseGPUTexture(res.device, res.texture);
        if (res.pipeline) {
            SDL_ReleaseGPUGraphicsPipeline(res.device, res.pipeline);
        }
    }
    res = OverlaySwEndResources{};
}

namespace {

std::string StringParameterOr(const WorkflowStepParameterResolver& params,
                              const WorkflowStepDefinition& step,
                              const char* key, const char* fallback) {
    const auto* p       = params.FindParameter(step, key);
    const bool isString = p && p->type == WorkflowParameterValue::Type::String;
    return isString ? p->stringValue : fallback;
}

}  // namespace

void ResolveOverlaySwEndShaderPaths(const WorkflowStepDefinition& step,
                                    SDL_GPUDevice* device,
                                    std::string& vertPath,
                                    std::string& fragPath) {
    WorkflowStepParameterResolver params;
    const char* driver           = SDL_GetGPUDeviceDriver(device);
    const std::string driverName = driver ? driver : "";
    if (driverName == "metal") {
        vertPath =
            StringParameterOr(params, step, "vert_shader_path_msl",
                              "packages/quake3/shaders/msl/overlay.vert.metal");
        fragPath =
            StringParameterOr(params, step, "frag_shader_path_msl",
                              "packages/quake3/shaders/msl/overlay.frag.metal");
    } else {
        vertPath =
            StringParameterOr(params, step, "vert_shader_path_spirv",
                              "packages/quake3/shaders/spirv/overlay.vert.spv");
        fragPath =
            StringParameterOr(params, step, "frag_shader_path_spirv",
                              "packages/quake3/shaders/spirv/overlay.frag.spv");
    }
}

}  // namespace sdl3cpp::services::impl
