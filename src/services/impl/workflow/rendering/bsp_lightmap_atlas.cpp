#include "services/interfaces/workflow/rendering/bsp_lightmap_atlas.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace sdl3cpp::services::impl {

LightmapAtlas BuildLightmapAtlas(const std::vector<uint8_t>& bspData) {
    auto* lumps =
        reinterpret_cast<const BspLump*>(bspData.data() + sizeof(BspHeader));

    const auto& lmLump = lumps[LUMP_LIGHTMAPS];
    const int numLightmaps =
        lmLump.length / (LM_BLOCK_SIZE * LM_BLOCK_SIZE * 3);
    const auto* lmData = bspData.data() + lmLump.offset;

    // Grid layout: ceil(sqrt(numLightmaps + 1)) — slot 0 is white for
    // lm_index=-1
    const int totalSlots = numLightmaps + 1;
    const int gridSize =
        static_cast<int>(std::ceil(std::sqrt(static_cast<float>(totalSlots))));

    LightmapAtlas atlas;
    atlas.gridSize = gridSize;
    atlas.numLightmaps = numLightmaps;
    atlas.width = gridSize * LM_BLOCK_SIZE;
    atlas.height = gridSize * LM_BLOCK_SIZE;
    atlas.pixels.assign(
        static_cast<size_t>(atlas.width) * atlas.height * 4, 0);

    // Slot 0: white (for faces with lm_index == -1)
    for (int y = 0; y < LM_BLOCK_SIZE; ++y) {
        for (int x = 0; x < LM_BLOCK_SIZE; ++x) {
            int dst = (y * atlas.width + x) * 4;
            atlas.pixels[dst + 0] = 255;
            atlas.pixels[dst + 1] = 255;
            atlas.pixels[dst + 2] = 255;
            atlas.pixels[dst + 3] = 255;
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
                int dstIdx = ((baseY + y) * atlas.width + (baseX + x)) * 4;

                atlas.pixels[dstIdx + 0] = static_cast<uint8_t>(
                    std::min(255, static_cast<int>(src[srcIdx + 0]) * 4));
                atlas.pixels[dstIdx + 1] = static_cast<uint8_t>(
                    std::min(255, static_cast<int>(src[srcIdx + 1]) * 4));
                atlas.pixels[dstIdx + 2] = static_cast<uint8_t>(
                    std::min(255, static_cast<int>(src[srcIdx + 2]) * 4));
                atlas.pixels[dstIdx + 3] = 255;
            }
        }
    }

    return atlas;
}

LightmapAtlasGpu UploadLightmapAtlas(SDL_GPUDevice* device,
                                     const LightmapAtlas& atlas) {
    LightmapAtlasGpu out;

    SDL_GPUTextureCreateInfo ti = {};
    ti.type = SDL_GPU_TEXTURETYPE_2D;
    ti.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    ti.width = static_cast<Uint32>(atlas.width);
    ti.height = static_cast<Uint32>(atlas.height);
    ti.layer_count_or_depth = 1;
    ti.num_levels = 1;
    ti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    out.texture = SDL_CreateGPUTexture(device, &ti);

    Uint32 dataSize = static_cast<Uint32>(atlas.pixels.size());
    SDL_GPUTransferBufferCreateInfo tbi = {};
    tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbi.size = dataSize;
    auto* tb = SDL_CreateGPUTransferBuffer(device, &tbi);
    auto* mapped =
        static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(device, tb, false));
    std::memcpy(mapped, atlas.pixels.data(), dataSize);
    SDL_UnmapGPUTransferBuffer(device, tb);

    auto* cmd = SDL_AcquireGPUCommandBuffer(device);
    auto* cp = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTextureTransferInfo srcInfo = {};
    srcInfo.transfer_buffer = tb;
    SDL_GPUTextureRegion dstRegion = {};
    dstRegion.texture = out.texture;
    dstRegion.w = static_cast<Uint32>(atlas.width);
    dstRegion.h = static_cast<Uint32>(atlas.height);
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
    out.sampler = SDL_CreateGPUSampler(device, &si);

    return out;
}

}  // namespace sdl3cpp::services::impl
