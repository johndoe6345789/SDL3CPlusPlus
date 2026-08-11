#include "services/interfaces/workflow/rendering/workflow_bsp_lightmap_atlas_step.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <cmath>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowBspLightmapAtlasStep::WorkflowBspLightmapAtlasStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBspLightmapAtlasStep::GetPluginId() const {
    return "bsp.lightmap_atlas";
}

void WorkflowBspLightmapAtlasStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    auto bspDataPtr = context.Get<std::shared_ptr<std::vector<uint8_t>>>("bsp_raw_data", nullptr);
    if (!bspDataPtr) throw std::runtime_error("bsp.lightmap_atlas: bsp_raw_data not in context");

    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) throw std::runtime_error("bsp.lightmap_atlas: GPU device not found");

    const auto& bspData = *bspDataPtr;
    auto* lumps = reinterpret_cast<const BspLump*>(bspData.data() + sizeof(BspHeader));

    const auto& lmLump = lumps[LUMP_LIGHTMAPS];
    int numLightmaps = lmLump.length / (LM_BLOCK_SIZE * LM_BLOCK_SIZE * 3);
    auto* lmData = bspData.data() + lmLump.offset;

    // Grid layout: ceil(sqrt(numLightmaps + 1)) — slot 0 is white for lm_index=-1
    int totalSlots = numLightmaps + 1;
    int gridSize = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(totalSlots))));
    int atlasW = gridSize * LM_BLOCK_SIZE;
    int atlasH = gridSize * LM_BLOCK_SIZE;

    // Build atlas in RGBA
    std::vector<uint8_t> atlasPixels(atlasW * atlasH * 4, 0);

    // Slot 0: white (for faces with lm_index == -1)
    {
        for (int y = 0; y < LM_BLOCK_SIZE; ++y) {
            for (int x = 0; x < LM_BLOCK_SIZE; ++x) {
                int dst = (y * atlasW + x) * 4;
                atlasPixels[dst + 0] = 255;
                atlasPixels[dst + 1] = 255;
                atlasPixels[dst + 2] = 255;
                atlasPixels[dst + 3] = 255;
            }
        }
    }

    // Copy lightmap blocks with overbright x4
    for (int lm = 0; lm < numLightmaps; ++lm) {
        int slot = lm + 1;
        int slotX = slot % gridSize;
        int slotY = slot / gridSize;
        int baseX = slotX * LM_BLOCK_SIZE;
        int baseY = slotY * LM_BLOCK_SIZE;

        const uint8_t* src = lmData + lm * LM_BLOCK_SIZE * LM_BLOCK_SIZE * 3;

        for (int y = 0; y < LM_BLOCK_SIZE; ++y) {
            for (int x = 0; x < LM_BLOCK_SIZE; ++x) {
                int srcIdx = (y * LM_BLOCK_SIZE + x) * 3;
                int dstIdx = ((baseY + y) * atlasW + (baseX + x)) * 4;

                atlasPixels[dstIdx + 0] = static_cast<uint8_t>(std::min(255, static_cast<int>(src[srcIdx + 0]) * 4));
                atlasPixels[dstIdx + 1] = static_cast<uint8_t>(std::min(255, static_cast<int>(src[srcIdx + 1]) * 4));
                atlasPixels[dstIdx + 2] = static_cast<uint8_t>(std::min(255, static_cast<int>(src[srcIdx + 2]) * 4));
                atlasPixels[dstIdx + 3] = 255;
            }
        }
    }

    // Upload lightmap atlas to GPU
    {
        SDL_GPUTextureCreateInfo ti = {};
        ti.type = SDL_GPU_TEXTURETYPE_2D;
        ti.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        ti.width = static_cast<Uint32>(atlasW);
        ti.height = static_cast<Uint32>(atlasH);
        ti.layer_count_or_depth = 1;
        ti.num_levels = 1;
        ti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        auto* lmTex = SDL_CreateGPUTexture(device, &ti);

        Uint32 dataSize = static_cast<Uint32>(atlasPixels.size());
        SDL_GPUTransferBufferCreateInfo tbi = {};
        tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbi.size = dataSize;
        auto* tb = SDL_CreateGPUTransferBuffer(device, &tbi);
        auto* mapped = static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(device, tb, false));
        std::memcpy(mapped, atlasPixels.data(), dataSize);
        SDL_UnmapGPUTransferBuffer(device, tb);

        auto* cmd = SDL_AcquireGPUCommandBuffer(device);
        auto* cp = SDL_BeginGPUCopyPass(cmd);
        SDL_GPUTextureTransferInfo srcInfo = {}; srcInfo.transfer_buffer = tb;
        SDL_GPUTextureRegion dstRegion = {};
        dstRegion.texture = lmTex;
        dstRegion.w = static_cast<Uint32>(atlasW);
        dstRegion.h = static_cast<Uint32>(atlasH);
        dstRegion.d = 1;
        SDL_UploadToGPUTexture(cp, &srcInfo, &dstRegion, false);
        SDL_EndGPUCopyPass(cp);
        SDL_SubmitGPUCommandBuffer(cmd);
        SDL_ReleaseGPUTransferBuffer(device, tb);

        SDL_GPUSamplerCreateInfo si = {};
        si.min_filter = SDL_GPU_FILTER_LINEAR;
        si.mag_filter = SDL_GPU_FILTER_LINEAR;
        si.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        auto* lmSamp = SDL_CreateGPUSampler(device, &si);

        context.Set<SDL_GPUTexture*>("bsp_lightmap_atlas_gpu", lmTex);
        context.Set<SDL_GPUSampler*>("bsp_lightmap_atlas_sampler", lmSamp);
    }

    // Store grid info for downstream steps
    context.Set("bsp_grid_size", gridSize);
    context.Set("bsp_num_lightmaps", numLightmaps);

    if (logger_) {
        logger_->Info("bsp.lightmap_atlas: " + std::to_string(atlasW) + "x" +
                     std::to_string(atlasH) + " (" + std::to_string(numLightmaps) +
                     " lightmaps, grid " + std::to_string(gridSize) + "x" + std::to_string(gridSize) + ")");
    }
}

}  // namespace sdl3cpp::services::impl
