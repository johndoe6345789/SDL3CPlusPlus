#include "services/interfaces/workflow/rendering/bsp_texture_batch_loader.hpp"
#include "services/interfaces/workflow/rendering/bsp_shader_script.hpp"
#include "services/interfaces/workflow/rendering/bsp_texture_gpu_upload.hpp"

namespace sdl3cpp::services::impl {
namespace {

void LoadOneTexture(zip_t* archive, SDL_GPUDevice* device, int texIdx,
                    const std::string& texName,
                    const std::map<std::string, std::string>& shaderImages,
                    WorkflowContext& context, BspTextureBatchResult& result) {
    const std::string texKey = "bsp_tex_" + std::to_string(texIdx) + "_gpu";
    const std::string sampKey =
        "bsp_tex_" + std::to_string(texIdx) + "_sampler";

    BspTextureUpload upload =
        LoadBspTextureFromPk3(archive, device, texName, shaderImages);
    if (!upload.texture) {
        upload = CreateBspWhiteTexture(device);
        ++result.missingCount;
    } else {
        ++result.loadedCount;
        if (upload.viaShader) ++result.viaShaderCount;
    }

    context.Set<SDL_GPUTexture*>(texKey, upload.texture);
    context.Set<SDL_GPUSampler*>(sampKey, upload.sampler);
}

}  // namespace

BspTextureBatchResult LoadBspTextureBatch(zip_t* archive, SDL_GPUDevice* device,
                                          const std::set<int>& usedTextures,
                                          const BspTexture* bspTextures,
                                          int numTextures,
                                          WorkflowContext& context) {
    const auto shaderImages = LoadShaderImages(archive);

    BspTextureBatchResult result;
    for (int texIdx : usedTextures) {
        if (texIdx < 0 || texIdx >= numTextures) {
            continue;
        }
        LoadOneTexture(archive, device, texIdx,
                       std::string(bspTextures[texIdx].name), shaderImages,
                       context, result);
    }
    return result;
}

}  // namespace sdl3cpp::services::impl
