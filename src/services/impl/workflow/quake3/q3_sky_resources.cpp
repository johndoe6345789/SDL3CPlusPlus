#include "services/interfaces/workflow/quake3/q3_sky_resources.hpp"

#include "services/interfaces/workflow/graphics/graphics_gpu_buffer_upload.hpp"
#include "services/interfaces/workflow/quake3/q3_sky_dome.hpp"
#include "services/interfaces/workflow/quake3/q3_sky_texture_name.hpp"
#include "services/interfaces/workflow/rendering/bsp_shader_script.hpp"
#include "services/interfaces/workflow/rendering/bsp_texture_gpu_upload.hpp"

#include <nlohmann/json.hpp>
#include <zip.h>

#include <cstring>
#include <exception>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

/// Comfortably outside any q3 map's extents but inside the camera's
/// 100-unit far plane, so the dome is never clipped away.
constexpr float kRadius   = 70.0f;
constexpr int kSegments   = 32;
constexpr int kRings      = 12;
/// tim_hell applies `tcMod scale 2 2` to its cloud stages.
constexpr float kUvScale = 2.0f;

}  // namespace

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

    const SkyDomeMesh mesh =
        BuildSkyDome(kRadius, kSegments, kRings, kUvScale);
    std::vector<uint8_t> vertexBytes(mesh.vertices.size() *
                                     sizeof(BspRenderVertex));
    std::memcpy(vertexBytes.data(), mesh.vertices.data(), vertexBytes.size());

    UploadedGpuBuffers buffers;
    try {
        buffers = CreateAndUploadGpuBuffers(device, vertexBytes, mesh.indices);
    } catch (const std::exception&) {
        return false;
    }

    const BspTextureUpload white = CreateBspWhiteTexture(device);
    out.cloudTex                 = cloud.texture;
    out.cloudSampler             = cloud.sampler;
    out.whiteTex                 = white.texture;
    out.whiteSampler             = white.sampler;
    out.vertexBuffer             = buffers.vertexBuffer;
    out.indexBuffer              = buffers.indexBuffer;
    out.indexCount               = static_cast<uint32_t>(mesh.indices.size());
    return true;
}

}  // namespace sdl3cpp::services::impl
