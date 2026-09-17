#include "services/interfaces/workflow/fs2024/fs2024_landmark_load.hpp"

#include "services/interfaces/workflow/fs2024/fs2024_landmark_kit.hpp"
#include "services/interfaces/workflow/graphics/texture_gpu_upload.hpp"
#include "services/interfaces/workflow/graphics/texture_image_io.hpp"
#include "services/interfaces/workflow/graphics/texture_load_sampler.hpp"
#include "services/interfaces/workflow/rendering/bsp_geometry_upload.hpp"

#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

std::vector<Fs2024LandmarkInstance> ReadTileLandmarks(
    const std::string& tileDir) {
    std::ifstream in(tileDir + "/landmarks.json");
    const auto doc = nlohmann::json::parse(in, nullptr, false);
    std::vector<Fs2024LandmarkInstance> instances;
    if (!doc.is_object()) return instances;
    for (const auto& item :
        doc.value("instances", nlohmann::json::array())) {
        Fs2024LandmarkInstance instance;
        instance.model = item.value("model", "");
        instance.x = item.value("x", 0.f);
        instance.z = item.value("z", 0.f);
        instance.headingDegrees = item.value("headingDegrees", 0.f);
        instances.push_back(std::move(instance));
    }
    return instances;
}

namespace {

void PlaceInWorld(std::vector<BspRenderVertex>& vertices, float x, float z,
                 float headingDegrees) {
    const float h = headingDegrees * 3.14159265f / 180.f;
    const float cosH = std::cos(h), sinH = std::sin(h);
    for (BspRenderVertex& v : vertices) {
        const float px = v.x, pz = v.z, nx = v.nx, nz = v.nz;
        v.x = px * cosH + pz * sinH + x;
        v.z = -px * sinH + pz * cosH + z;
        v.nx = nx * cosH + nz * sinH;
        v.nz = -nx * sinH + nz * cosH;
    }
}

}  // namespace

Fs2024LandmarkKitGpu LoadFs2024LandmarkKitGpu(SDL_GPUDevice* device,
                                            const std::string& tilesRoot,
                                            const std::string& model,
                                            float x, float z,
                                            float headingDegrees) {
    auto meshGroups =
        ReadLandmarkKit(tilesRoot + "/landmarks/" + model + ".lmk");
    const std::string texturesDir = tilesRoot + "/landmarks/textures/";

    Fs2024LandmarkKitGpu kit;
    kit.groups.reserve(meshGroups.size());
    for (LandmarkMeshGroup& meshGroup : meshGroups) {
        if (meshGroup.mesh.indices.empty()) continue;
        PlaceInWorld(meshGroup.mesh.vertices, x, z, headingDegrees);
        Fs2024LandmarkGroupGpu group;
        const BspGeometryBuffers buffers = UploadBspGeometryBuffers(
            device, meshGroup.mesh.vertices, meshGroup.mesh.indices);
        group.vertexBuffer = buffers.vertex_buffer;
        group.indexBuffer = buffers.index_buffer;
        group.indexCount =
            static_cast<std::uint32_t>(meshGroup.mesh.indices.size());
        if (!meshGroup.textureFile.empty()) {
            LoadedTextureImage image =
                LoadTextureImagePixels(texturesDir + meshGroup.textureFile);
            const UploadedTexture uploaded = UploadTextureImage(device,
                                                                image);
            group.texture = uploaded.texture;
            group.sampler = CreateTextureLoadSampler(device, uploaded.texture,
                                                     uploaded.numLevels);
        }
        kit.groups.push_back(group);
    }
    return kit;
}

void ReleaseFs2024LandmarkKitGpu(SDL_GPUDevice* device,
                                Fs2024LandmarkKitGpu& kit) {
    if (!device) return;
    for (Fs2024LandmarkGroupGpu& group : kit.groups) {
        SDL_ReleaseGPUBuffer(device, group.vertexBuffer);
        SDL_ReleaseGPUBuffer(device, group.indexBuffer);
        SDL_ReleaseGPUTexture(device, group.texture);
        SDL_ReleaseGPUSampler(device, group.sampler);
    }
    kit.groups.clear();
}

}  // namespace sdl3cpp::services::impl
