#include "services/interfaces/workflow/fs2024/world/fs2024_world_gpu.hpp"

#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"
#include "services/interfaces/workflow/fs2024/data/texture/fs2024_dds_array_blocks.hpp"
#include "services/interfaces/workflow/fs2024/data/texture/fs2024_pgg_kit_extract.hpp"
#include "services/interfaces/workflow/graphics/texture_array_gpu_upload.hpp"
#include "services/interfaces/workflow/graphics/texture_load_sampler.hpp"

#include <cstdlib>
#include <cstring>
#include <string>

namespace sdl3cpp::services::impl {
namespace {

SDL_GPUTextureFormat Bc1Format(std::uint32_t dxgi) {
    return dxgi == sdl3cpp::fs2024::kDxgiBc1UnormSrgb
               ? SDL_GPU_TEXTUREFORMAT_BC1_RGBA_UNORM_SRGB
               : SDL_GPU_TEXTUREFORMAT_BC1_RGBA_UNORM;
}

/// Uploads one RGBA8 image with its mip chain and publishes it as
/// `<key>_gpu` / `<key>_sampler`.
void Publish(SDL_GPUDevice* device, const sdl3cpp::fs2024::DdsImage& image,
             const std::string& key, WorkflowContext& context) {
    LoadedTextureImage pixels;
    pixels.width = image.width;
    pixels.height = image.height;
    pixels.pixels = static_cast<unsigned char*>(std::malloc(image.rgba.size()));
    std::memcpy(pixels.pixels, image.rgba.data(), image.rgba.size());
    const UploadedTexture uploaded = UploadTextureImage(device, pixels);
    context.Set<SDL_GPUTexture*>(key + "_gpu", uploaded.texture);
    context.Set<SDL_GPUSampler*>(
        key + "_sampler",
        CreateTextureLoadSampler(device, uploaded.texture, uploaded.numLevels));
}

}  // namespace

void UploadFs2024GroundMaterials(SDL_GPUDevice* device, Fs2024World& world,
                                 int climate) {
    const std::string autogen = world.paths.texSynthRoot + "/autogen/";
    world.materials =
        sdl3cpp::fs2024::Fs2024GroundMaterials::Read(autogen + "arrays.xml");
    const auto blocks =
        sdl3cpp::fs2024::ReadDdsArrayBlocks(autogen + "array_low.dds");
    TextureArrayBlocksView view;
    view.format = Bc1Format(blocks.info.dxgiFormat);
    view.width = blocks.info.width;
    view.height = blocks.info.height;
    view.layers = blocks.info.layers;
    view.mips = blocks.info.mips;
    view.blockBytes = blocks.info.blockBytes;
    view.data = blocks.data.data();
    view.size = blocks.data.size();
    const UploadedTexture uploaded = UploadTextureArrayBlocks(device, view);
    world.materialArray = uploaded.texture;
    world.materialSampler =
        CreateTextureLoadSampler(device, uploaded.texture, uploaded.numLevels);
    for (int landClass = 0; landClass < kFs2024LandClasses; ++landClass) {
        const auto material = world.materials.For(landClass, climate);
        world.materialTable[landClass] =
            glm::vec4(material.firstLayer, material.layers, 0.f, 0.f);
    }
}

void PublishFs2024BuildingKit(SDL_GPUDevice* device, const Fs2024World& world,
                              WorkflowContext& context) {
    const auto kit =
        sdl3cpp::fs2024::BakePggBuildingKit(world.paths.pggRoot + "/textures");
    Publish(device, kit.wall, "fs2024_building", context);
    Publish(device, kit.roof, "fs2024_roof", context);
}

}  // namespace sdl3cpp::services::impl
