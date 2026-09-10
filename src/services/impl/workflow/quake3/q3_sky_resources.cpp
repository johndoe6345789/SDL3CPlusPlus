#include "services/interfaces/workflow/quake3/q3_sky_resources.hpp"

#include "services/interfaces/workflow/quake3/q3_sky_dome_upload.hpp"
#include "services/interfaces/workflow/quake3/q3_sky_texture_name.hpp"
#include "services/interfaces/workflow/rendering/bsp_shader_script.hpp"
#include "services/interfaces/workflow/rendering/bsp_texture_gpu_upload.hpp"

#include <nlohmann/json.hpp>
#include <zip.h>

#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

bool InitSkyResources(WorkflowContext& context, SkyResources& out) {
    out.attempted = true;

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    const auto bspConfig =
        context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    const std::string pk3 = bspConfig.value("pk3_path", std::string());
    const std::string skyName = FindSkyTextureName(
        context.Get<std::shared_ptr<std::vector<uint8_t>>>("bsp_raw_data",
                                                           nullptr));
    if (!device || pk3.empty() || skyName.empty()) {
        return false;
    }

    int err        = 0;
    zip_t* archive = zip_open(pk3.c_str(), ZIP_RDONLY, &err);
    if (!archive) {
        return false;
    }
    const BspTextureUpload cloud = LoadBspTextureFromPk3(
        archive, device, skyName, LoadShaderImages(archive));
    zip_close(archive);
    if (!cloud.texture) {
        return false;
    }

    if (!UploadSkyDome(device, out)) {
        return false;
    }

    const BspTextureUpload white = CreateBspWhiteTexture(device);
    out.cloudTex                 = cloud.texture;
    out.cloudSampler             = cloud.sampler;
    out.whiteTex                 = white.texture;
    out.whiteSampler             = white.sampler;
    return true;
}

}  // namespace sdl3cpp::services::impl
