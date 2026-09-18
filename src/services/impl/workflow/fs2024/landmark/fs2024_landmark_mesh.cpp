#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_mesh.hpp"

#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_place.hpp"

#include <exception>
#include <filesystem>

namespace sdl3cpp::services::impl {
namespace {

namespace f = sdl3cpp::fs2024;

std::shared_ptr<const f::DdsBlocks> ReadMap(const std::string& path) {
    if (!std::filesystem::exists(path)) return nullptr;
    try {
        return std::make_shared<const f::DdsBlocks>(f::ReadDdsBlocks(path));
    } catch (const std::exception&) {
        return nullptr;  // one bad map should not lose the landmark
    }
}

}  // namespace

Fs2024LandmarkMesh ReadFs2024LandmarkMesh(const std::string& library,
                                          const f::ModelLibraryEntry& entry,
                                          const std::string& texturesDir,
                                          std::size_t lodBudgetBytes) {
    Fs2024LandmarkMesh mesh;
    mesh.name = entry.name;
    const auto riff = f::ReadModelRiff(library, entry);
    const auto lods = f::ListModelRiffLods(riff);
    if (lods.empty()) return mesh;
    mesh.primitives =
        f::ParseModelRiffLod(riff, ChooseFs2024LandmarkLod(lods,
                                                           lodBudgetBytes))
            .primitives;
    mesh.bounds = {glm::vec3(1e30f), glm::vec3(-1e30f)};
    for (const f::GltfPrimitive& prim : mesh.primitives) {
        for (const BspRenderVertex& v : prim.mesh.vertices) {
            const glm::vec3 p(v.x, v.y, v.z);
            mesh.bounds.min = glm::min(mesh.bounds.min, p);
            mesh.bounds.max = glm::max(mesh.bounds.max, p);
        }
        const std::string& uri = prim.baseColorImageUri;
        if (uri.empty() || mesh.textures.count(uri)) continue;
        if (auto map = ReadMap(texturesDir + "/" + uri)) {
            mesh.textures.emplace(uri, std::move(map));
        }
    }
    if (mesh.bounds.min.x > mesh.bounds.max.x) mesh.bounds = {};
    return mesh;
}

}  // namespace sdl3cpp::services::impl
