#include "services/interfaces/workflow/rendering/gpu_text_overlay_resources.hpp"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

constexpr const char* kVertSpvPath =
    "packages/quake3/shaders/spirv/overlay.vert.spv";
constexpr const char* kFragSpvPath =
    "packages/quake3/shaders/spirv/overlay.frag.spv";

std::vector<uint8_t> LoadSpirv(const char* path) {
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

SDL_GPUShader* CreateShader(SDL_GPUDevice* device,
                            const std::vector<uint8_t>& code,
                            SDL_GPUShaderStage stage, Uint32 numSamplers) {
    SDL_GPUShaderCreateInfo info = {};
    info.code                    = code.data();
    info.code_size               = code.size();
    info.entrypoint              = "main";
    info.format                  = SDL_GPU_SHADERFORMAT_SPIRV;
    info.stage                   = stage;
    info.num_samplers            = numSamplers;
    return SDL_CreateGPUShader(device, &info);
}

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

SDL_GPUTexture* CreateOverlayTexture(SDL_GPUDevice* device) {
    SDL_GPUTextureCreateInfo tci = {};
    tci.type                     = SDL_GPU_TEXTURETYPE_2D;
    tci.format                   = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tci.usage                    = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    tci.width                    = kGpuTextOverlayWidth;
    tci.height                   = kGpuTextOverlayHeight;
    tci.layer_count_or_depth     = 1;
    tci.num_levels               = 1;
    return SDL_CreateGPUTexture(device, &tci);
}

SDL_GPUSampler* CreateOverlaySampler(SDL_GPUDevice* device) {
    SDL_GPUSamplerCreateInfo sci = {};
    sci.min_filter               = SDL_GPU_FILTER_NEAREST;
    sci.mag_filter               = SDL_GPU_FILTER_NEAREST;
    sci.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sci.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sci.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    return SDL_CreateGPUSampler(device, &sci);
}

}  // namespace

const char* CreateGpuTextOverlayResources(SDL_GPUDevice* device,
                                          SDL_GPUTextureFormat swapchainFmt,
                                          GpuTextOverlayResources& out) {
    if (!device) {
        return "no GPU device";
    }

    // The overlay shaders are only shipped as SPIR-V; the Metal backend would
    // need MSL variants before this could run there.
    const char* driver = SDL_GetGPUDeviceDriver(device);
    if (driver && std::string(driver) != "vulkan") {
        return "overlay only supported on Vulkan";
    }

    const std::vector<uint8_t> vertCode = LoadSpirv(kVertSpvPath);
    const std::vector<uint8_t> fragCode = LoadSpirv(kFragSpvPath);
    if (vertCode.empty() || fragCode.empty()) {
        return "overlay shaders not found";
    }

    SDL_GPUShader* vertex =
        CreateShader(device, vertCode, SDL_GPU_SHADERSTAGE_VERTEX, 0);
    SDL_GPUShader* fragment =
        CreateShader(device, fragCode, SDL_GPU_SHADERSTAGE_FRAGMENT, 1);
    if (!vertex || !fragment) {
        if (vertex) SDL_ReleaseGPUShader(device, vertex);
        if (fragment) SDL_ReleaseGPUShader(device, fragment);
        return "overlay shader creation failed";
    }

    GpuTextOverlayResources res;
    res.device = device;
    res.pipeline =
        CreateOverlayPipeline(device, swapchainFmt, vertex, fragment);
    SDL_ReleaseGPUShader(device, vertex);
    SDL_ReleaseGPUShader(device, fragment);
    if (!res.pipeline) {
        return "overlay pipeline creation failed";
    }

    res.texture = CreateOverlayTexture(device);
    if (!res.texture) {
        DestroyGpuTextOverlayResources(res);
        return "overlay texture creation failed";
    }

    SDL_GPUTransferBufferCreateInfo tbci = {};
    tbci.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size =
        static_cast<Uint32>(kGpuTextOverlayWidth * kGpuTextOverlayHeight * 4);
    res.transfer = SDL_CreateGPUTransferBuffer(device, &tbci);
    if (!res.transfer) {
        DestroyGpuTextOverlayResources(res);
        return "overlay transfer buffer creation failed";
    }

    SDL_GPUBufferCreateInfo bci = {};
    bci.usage                   = SDL_GPU_BUFFERUSAGE_VERTEX;
    bci.size                    = 6u * 5u * static_cast<Uint32>(sizeof(float));
    res.vertices                = SDL_CreateGPUBuffer(device, &bci);
    if (!res.vertices) {
        DestroyGpuTextOverlayResources(res);
        return "overlay vertex buffer creation failed";
    }

    res.sampler = CreateOverlaySampler(device);
    if (!res.sampler) {
        DestroyGpuTextOverlayResources(res);
        return "overlay sampler creation failed";
    }

    res.surface = SDL_CreateSurface(kGpuTextOverlayWidth, kGpuTextOverlayHeight,
                                    SDL_PIXELFORMAT_RGBA32);
    if (!res.surface) {
        DestroyGpuTextOverlayResources(res);
        return "overlay surface creation failed";
    }

    res.renderer = SDL_CreateSoftwareRenderer(res.surface);
    if (!res.renderer) {
        DestroyGpuTextOverlayResources(res);
        return "overlay software renderer creation failed";
    }

    out = res;
    return "";
}

void DestroyGpuTextOverlayResources(GpuTextOverlayResources& res) {
    if (res.renderer) {
        SDL_DestroyRenderer(res.renderer);
        res.renderer = nullptr;
    }
    if (res.surface) {
        SDL_DestroySurface(res.surface);
        res.surface = nullptr;
    }
    if (res.device) {
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
    res.sampler  = nullptr;
    res.vertices = nullptr;
    res.transfer = nullptr;
    res.texture  = nullptr;
    res.pipeline = nullptr;
    res.device   = nullptr;
}

}  // namespace sdl3cpp::services::impl
