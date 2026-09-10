#include "services/interfaces/workflow/quake3/q3_md3_surface_uploader.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_gpu_upload.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_texture_candidates.hpp"

#include <cstring>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

/// MD3 triangles are already counter-clockwise, so the winding is preserved.
std::vector<uint16_t> ReadIndices(const uint8_t* surfacePtr,
                                  const q3::Md3Surface& surface) {
    const auto* tris = reinterpret_cast<const q3::Md3Triangle*>(
        surfacePtr + surface.ofsTriangles);
    std::vector<uint16_t> indices;
    indices.reserve(static_cast<size_t>(surface.numTriangles) * 3);
    for (int i = 0; i < surface.numTriangles; ++i) {
        indices.push_back(static_cast<uint16_t>(tris[i].indexes[0]));
        indices.push_back(static_cast<uint16_t>(tris[i].indexes[1]));
        indices.push_back(static_cast<uint16_t>(tris[i].indexes[2]));
    }
    return indices;
}

/// Decodes one frame's vertices into engine space, baking in the world scale.
void DecodeFrame(const q3::Md3XyzNormal* xyz, const q3::Md3St* uvs,
                 int vertexCount, std::vector<q3::Md3PosUv>& out) {
    constexpr float kScale = q3::kMd3XyzScale * q3::kMd3WorldScale;
    for (int v = 0; v < vertexCount; ++v) {
        out[v].x = xyz[v].xyz[0] * kScale;
        out[v].y = xyz[v].xyz[2] * kScale;   // Q3 Z -> engine Y
        out[v].z = -xyz[v].xyz[1] * kScale;  // -Q3 Y -> engine Z
        out[v].u = uvs[v].st[0];
        out[v].v = uvs[v].st[1];
    }
}

std::string SurfaceShaderName(const uint8_t* surfacePtr,
                              const q3::Md3Surface& surface) {
    if (surface.numShaders <= 0) {
        return {};
    }
    const auto* shaders =
        reinterpret_cast<const q3::Md3Shader*>(surfacePtr + surface.ofsShaders);
    return std::string(shaders[0].name, strnlen(shaders[0].name, 64));
}

void UploadIndexBuffer(SDL_GPUDevice* device, const uint8_t* surfacePtr,
                       const q3::Md3Surface& surface,
                       const std::string& keyPrefix, WorkflowContext& context) {
    const std::vector<uint16_t> indices = ReadIndices(surfacePtr, surface);
    context.Set<SDL_GPUBuffer*>(
        keyPrefix + "_ib",
        q3::UploadMd3Buffer(device, SDL_GPU_BUFFERUSAGE_INDEX, indices.data(),
                            static_cast<uint32_t>(indices.size() * 2)));
    context.Set<int>(keyPrefix + "_num_idx", static_cast<int>(indices.size()));
}

void UploadVertexBuffers(SDL_GPUDevice* device, const uint8_t* surfacePtr,
                         const q3::Md3Surface& surface, int frameCount,
                         const std::string& keyPrefix,
                         WorkflowContext& context) {
    const auto* uvs =
        reinterpret_cast<const q3::Md3St*>(surfacePtr + surface.ofsSt);
    const auto* xyz = reinterpret_cast<const q3::Md3XyzNormal*>(
        surfacePtr + surface.ofsXyzNormals);
    const int vertexCount = surface.numVerts;
    std::vector<q3::Md3PosUv> vertices(static_cast<size_t>(vertexCount));

    for (int f = 0; f < frameCount; ++f) {
        DecodeFrame(xyz + f * vertexCount, uvs, vertexCount, vertices);
        context.Set<SDL_GPUBuffer*>(
            keyPrefix + "_f" + std::to_string(f) + "_vb",
            q3::UploadMd3Buffer(
                device, SDL_GPU_BUFFERUSAGE_VERTEX, vertices.data(),
                static_cast<uint32_t>(vertexCount * sizeof(q3::Md3PosUv))));
    }
}

void UploadSurfaceTexture(SDL_GPUDevice* device, const Q3Md3Source& source,
                          const uint8_t* surfacePtr,
                          const q3::Md3Surface& surface,
                          const std::string& keyPrefix,
                          WorkflowContext& context) {
    const std::string surfaceName(surface.name, strnlen(surface.name, 64));
    auto* texture = q3::TryLoadMd3Texture(
        device, source.pk3Path,
        q3::BuildMd3TextureCandidates(
            q3::SkinTextureFor(source.skinMap, surfaceName),
            SurfaceShaderName(surfacePtr, surface), source.skin, source.path));
    if (!texture) {
        const uint8_t grey[4] = {128, 128, 128, 255};
        texture               = q3::UploadMd3Texture(device, grey, 1, 1);
    }
    context.Set<SDL_GPUTexture*>(keyPrefix + "_tex", texture);
    context.Set<SDL_GPUSampler*>(keyPrefix + "_samp",
                                 q3::MakeMd3LinearSampler(device));
}

}  // namespace

void UploadMd3Surface(SDL_GPUDevice* device, const Q3Md3Source& source,
                      const uint8_t* surfacePtr, const q3::Md3Surface& surface,
                      int frameCount, const std::string& keyPrefix,
                      WorkflowContext& context) {
    if (surface.numVerts <= 0 || surface.numTriangles <= 0) {
        return;
    }
    UploadIndexBuffer(device, surfacePtr, surface, keyPrefix, context);
    UploadVertexBuffers(device, surfacePtr, surface, frameCount, keyPrefix,
                        context);
    UploadSurfaceTexture(device, source, surfacePtr, surface, keyPrefix,
                         context);
}

}  // namespace sdl3cpp::services::impl
