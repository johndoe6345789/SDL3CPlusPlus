#include "services/interfaces/workflow/fs2024/terrain/fs2024_vegetation_gpu.hpp"

#include "services/interfaces/workflow/fs2024/data/texture/fs2024_dds_array_blocks.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"
#include "services/interfaces/workflow/graphics/texture_array_gpu_upload.hpp"
#include "services/interfaces/workflow/graphics/texture_load_sampler.hpp"
#include "services/interfaces/workflow/rendering/bsp_geometry_upload.hpp"

namespace sdl3cpp::services::impl {
namespace {

SDL_GPUTextureFormat Bc7Format(std::uint32_t dxgi) {
    return dxgi == sdl3cpp::fs2024::kDxgiBc7UnormSrgb
               ? SDL_GPU_TEXTUREFORMAT_BC7_RGBA_UNORM_SRGB
               : SDL_GPU_TEXTUREFORMAT_BC7_RGBA_UNORM;
}

}  // namespace

Fs2024VegSpeciesGpu UploadFs2024VegetationSpecies(SDL_GPUDevice* device,
                                                  const std::string& path) {
    const auto blocks = sdl3cpp::fs2024::ReadDdsArrayBlocks(path);
    TextureArrayBlocksView view;
    view.type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
    view.format = Bc7Format(blocks.info.dxgiFormat);
    view.width = blocks.info.width;
    view.height = blocks.info.height;
    view.layers = blocks.info.layers;
    view.mips = blocks.info.mips;
    view.blockBytes = blocks.info.blockBytes;
    view.data = blocks.data.data();
    view.size = blocks.data.size();
    const UploadedTexture uploaded = UploadTextureArrayBlocks(device, view);
    Fs2024VegSpeciesGpu species;
    species.texture = uploaded.texture;
    species.sampler =
        CreateTextureLoadSampler(device, uploaded.texture, uploaded.numLevels);
    return species;
}

std::vector<Fs2024VegetationChunkGpu> FinishFs2024TileVegetation(
    SDL_GPUDevice* device, const Fs2024PreparedTile& prepared,
    Fs2024VegSpeciesTextures& species) {
    std::vector<Fs2024VegetationChunkGpu> chunks;
    for (const Fs2024VegetationGroupCpu& group : prepared.vegetation) {
        if (group.mesh.indices.empty()) continue;
        if (!species.count(group.speciesName)) {
            species.emplace(group.speciesName,
                            UploadFs2024VegetationSpecies(device,
                                                          group.albedoPath));
        }
        const BspGeometryBuffers buffers = UploadBspGeometryBuffers(
            device, group.mesh.vertices, group.mesh.indices);
        Fs2024VegetationChunkGpu chunk;
        chunk.speciesName = group.speciesName;
        chunk.vertexBuffer = buffers.vertex_buffer;
        chunk.indexBuffer = buffers.index_buffer;
        chunk.indexCount =
            static_cast<std::uint32_t>(group.mesh.indices.size());
        chunks.push_back(chunk);
    }
    return chunks;
}

}  // namespace sdl3cpp::services::impl
