#include "services/interfaces/workflow/graphics/workflow_texture_load_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <stb_image.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <cstdlib>

namespace sdl3cpp::services::impl {

WorkflowTextureLoadStep::WorkflowTextureLoadStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowTextureLoadStep::GetPluginId() const {
    return "texture.load";
}

void WorkflowTextureLoadStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string pathKey = resolver.GetRequiredInputKey(step, "image_path");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "texture");

    const auto* image_path = context.TryGet<std::string>(pathKey);
    if (!image_path) {
        throw std::runtime_error("texture.load: image_path not found in context key '" + pathKey + "'");
    }

    // Resolve ~ paths
    std::string resolved = *image_path;
    if (!resolved.empty() && resolved[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) resolved = std::string(home) + resolved.substr(1);
    }

    if (logger_) {
        logger_->Trace("WorkflowTextureLoadStep", "Execute", "path=" + resolved, "Loading texture");
    }

    // Load image with stb_image (force RGBA)
    int w = 0, h = 0, channels = 0;
    unsigned char* pixels = stbi_load(resolved.c_str(), &w, &h, &channels, 4);
    if (!pixels) {
        throw std::runtime_error("texture.load: Failed to load image: " + resolved +
                                 " (" + std::string(stbi_failure_reason()) + ")");
    }

    // Get GPU device
    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        stbi_image_free(pixels);
        throw std::runtime_error("texture.load: GPU device not found in context");
    }

    // Calculate mip levels: floor(log2(max(w,h))) + 1
    int maxDim = std::max(w, h);
    Uint32 numLevels = 1;
    while (maxDim > 1) { maxDim >>= 1; numLevels++; }

    // Create GPU texture with mip chain
    SDL_GPUTextureCreateInfo tex_info = {};
    tex_info.type = SDL_GPU_TEXTURETYPE_2D;
    tex_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tex_info.width = static_cast<Uint32>(w);
    tex_info.height = static_cast<Uint32>(h);
    tex_info.layer_count_or_depth = 1;
    tex_info.num_levels = numLevels;
    tex_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER |
                      (numLevels > 1 ? SDL_GPU_TEXTUREUSAGE_COLOR_TARGET : 0);

    SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &tex_info);
    if (!texture) {
        stbi_image_free(pixels);
        throw std::runtime_error("texture.load: SDL_CreateGPUTexture failed: " +
                                 std::string(SDL_GetError()));
    }

    // Upload via transfer buffer
    const Uint32 data_size = static_cast<Uint32>(w * h * 4);

    SDL_GPUTransferBufferCreateInfo tbuf_info = {};
    tbuf_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbuf_info.size = data_size;

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &tbuf_info);
    if (!transfer) {
        stbi_image_free(pixels);
        SDL_ReleaseGPUTexture(device, texture);
        throw std::runtime_error("texture.load: Failed to create transfer buffer");
    }

    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    std::memcpy(mapped, pixels, data_size);
    SDL_UnmapGPUTransferBuffer(device, transfer);
    stbi_image_free(pixels);

    // Copy to GPU texture
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTextureTransferInfo src = {};
    src.transfer_buffer = transfer;
    src.offset = 0;

    SDL_GPUTextureRegion dst = {};
    dst.texture = texture;
    dst.w = static_cast<Uint32>(w);
    dst.h = static_cast<Uint32>(h);
    dst.d = 1;

    SDL_UploadToGPUTexture(copy_pass, &src, &dst, false);
    SDL_EndGPUCopyPass(copy_pass);

    // Generate mipmaps from the uploaded base level
    if (numLevels > 1) {
        SDL_GenerateMipmapsForGPUTexture(cmd, texture);
    }

    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    // Create sampler (linear filtering, repeat wrap for tiling)
    SDL_GPUSamplerCreateInfo samp_info = {};
    samp_info.min_filter = SDL_GPU_FILTER_LINEAR;
    samp_info.mag_filter = SDL_GPU_FILTER_LINEAR;
    samp_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    samp_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samp_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samp_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samp_info.enable_anisotropy = true;
    samp_info.max_anisotropy = 16.0f;
    samp_info.mip_lod_bias = 0.5f;  // bias toward higher mip = less aliasing at distance
    samp_info.min_lod = 0.0f;
    samp_info.max_lod = static_cast<float>(numLevels);

    SDL_GPUSampler* sampler = SDL_CreateGPUSampler(device, &samp_info);
    if (!sampler) {
        SDL_ReleaseGPUTexture(device, texture);
        throw std::runtime_error("texture.load: SDL_CreateGPUSampler failed");
    }

    // Store in context — use output key prefix for texture and sampler
    context.Set<SDL_GPUTexture*>(outputKey + "_gpu", texture);
    context.Set<SDL_GPUSampler*>(outputKey + "_sampler", sampler);

    // Store metadata as JSON
    nlohmann::json meta = {
        {"valid", true},
        {"width", w},
        {"height", h},
        {"channels", 4},
        {"path", resolved}
    };
    context.Set(outputKey, meta);

    if (logger_) {
        logger_->Info("texture.load: Loaded " + resolved + " (" +
                     std::to_string(w) + "x" + std::to_string(h) + ", " +
                     std::to_string(data_size) + " bytes)");
    }
}

}  // namespace sdl3cpp::services::impl
