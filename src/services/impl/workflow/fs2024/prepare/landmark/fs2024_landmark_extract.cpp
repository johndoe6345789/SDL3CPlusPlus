#include "services/interfaces/workflow/fs2024/prepare/landmark/fs2024_landmark_extract.hpp"

#include "services/interfaces/workflow/fs2024/prepare/bgl/fs2024_bgl_model_library.hpp"
#include "services/interfaces/workflow/fs2024/prepare/texture/fs2024_dds_texture.hpp"
#include "services/interfaces/workflow/fs2024/prepare/gltf/fs2024_gltf_model.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>
#include <stb_image_write.h>
#include <stdexcept>

namespace sdl3cpp::tools::fs2024 {
namespace {
namespace fs = std::filesystem;
using services::impl::BspRenderVertex;

/// Decodes every primitive's base-colour texture the first time it is
/// seen, skipping ones already converted (shared across landmarks, or
/// a rerun of the same bake).
void WriteTextures(const std::vector<GltfPrimitive>& primitives,
                  const std::string& texturesDir, const fs::path& outDir) {
    std::set<std::string> seen;
    for (const auto& prim : primitives) {
        if (prim.baseColorImageUri.empty()) continue;
        if (!seen.insert(prim.baseColorImageUri).second) continue;
        const fs::path outPath = outDir / (prim.baseColorImageUri + ".png");
        if (fs::exists(outPath)) continue;
        const auto image =
            DecodeDds(texturesDir + "/" + prim.baseColorImageUri);
        stbi_write_png(outPath.string().c_str(), image.width, image.height,
                      4, image.rgba.data(), image.width * 4);
    }
}

void WriteLmk(const fs::path& path,
             const std::vector<GltfPrimitive>& primitives) {
    std::ofstream out(path, std::ios::binary);
    out.write("LMK1", 4);
    const auto groupCount = static_cast<std::uint32_t>(primitives.size());
    out.write(reinterpret_cast<const char*>(&groupCount), 4);
    for (const auto& prim : primitives) {
        const std::string textureFile = prim.baseColorImageUri.empty()
            ? std::string()
            : prim.baseColorImageUri + ".png";
        const auto nameLen = static_cast<std::uint32_t>(textureFile.size());
        out.write(reinterpret_cast<const char*>(&nameLen), 4);
        out.write(textureFile.data(),
                 static_cast<std::streamsize>(textureFile.size()));

        const auto vertexCount =
            static_cast<std::uint32_t>(prim.mesh.vertices.size());
        out.write(reinterpret_cast<const char*>(&vertexCount), 4);
        out.write(reinterpret_cast<const char*>(prim.mesh.vertices.data()),
                 static_cast<std::streamsize>(vertexCount *
                                              sizeof(BspRenderVertex)));

        const auto indexCount =
            static_cast<std::uint32_t>(prim.mesh.indices.size());
        out.write(reinterpret_cast<const char*>(&indexCount), 4);
        out.write(reinterpret_cast<const char*>(prim.mesh.indices.data()),
                 static_cast<std::streamsize>(indexCount * sizeof(std::uint32_t)));
    }
}

}  // namespace

void ExtractLandmarkKit(const LandmarkCatalogEntry& entry,
                       const std::string& outDir) {
    const fs::path landmarksDir = fs::path(outDir) / "landmarks";
    const fs::path lmkPath = landmarksDir / (entry.model + ".lmk");
    if (fs::exists(lmkPath)) return;

    const auto models = ListModelLibrary(entry.bglPath);
    const auto it = std::find_if(
        models.begin(), models.end(),
        [&](const ModelLibraryEntry& m) { return m.name == entry.model; });
    if (it == models.end()) {
        throw std::runtime_error("landmark '" + entry.model +
                                 "' not found in " + entry.bglPath);
    }

    const auto riff = ReadModelRiff(entry.bglPath, *it);
    const auto lods = ParseModelRiff(riff);
    if (lods.empty()) {
        throw std::runtime_error("landmark '" + entry.model + "' has no "
                                 "LODs");
    }

    const fs::path texturesOutDir = landmarksDir / "textures";
    fs::create_directories(texturesOutDir);
    WriteTextures(lods[0].primitives, entry.texturesDir, texturesOutDir);
    WriteLmk(lmkPath, lods[0].primitives);
    std::printf("landmark '%s': %zu primitives extracted\n",
               entry.model.c_str(), lods[0].primitives.size());
}

}  // namespace sdl3cpp::tools::fs2024
