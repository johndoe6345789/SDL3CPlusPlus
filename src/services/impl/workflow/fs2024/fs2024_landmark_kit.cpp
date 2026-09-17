#include "services/interfaces/workflow/fs2024/fs2024_landmark_kit.hpp"

#include <cstring>
#include <fstream>
#include <stdexcept>

namespace sdl3cpp::services::impl {

std::vector<LandmarkMeshGroup> ReadLandmarkKit(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("landmark kit '" + path + "': open");

    char magic[4] = {};
    in.read(magic, 4);
    if (!in || std::memcmp(magic, "LMK1", 4) != 0) {
        throw std::runtime_error("landmark kit '" + path + "': bad magic");
    }
    std::uint32_t groupCount = 0;
    in.read(reinterpret_cast<char*>(&groupCount), 4);

    std::vector<LandmarkMeshGroup> groups(groupCount);
    for (auto& group : groups) {
        std::uint32_t nameLen = 0;
        in.read(reinterpret_cast<char*>(&nameLen), 4);
        group.textureFile.resize(nameLen);
        in.read(group.textureFile.data(), nameLen);

        std::uint32_t vertexCount = 0;
        in.read(reinterpret_cast<char*>(&vertexCount), 4);
        group.mesh.vertices.resize(vertexCount);
        in.read(reinterpret_cast<char*>(group.mesh.vertices.data()),
               static_cast<std::streamsize>(vertexCount *
                                            sizeof(BspRenderVertex)));

        std::uint32_t indexCount = 0;
        in.read(reinterpret_cast<char*>(&indexCount), 4);
        group.mesh.indices.resize(indexCount);
        in.read(reinterpret_cast<char*>(group.mesh.indices.data()),
               static_cast<std::streamsize>(indexCount *
                                            sizeof(std::uint32_t)));
    }
    if (!in) throw std::runtime_error("landmark kit '" + path + "': short");
    return groups;
}

}  // namespace sdl3cpp::services::impl
