#include "services/interfaces/workflow/fs2024/data/landmark/fs2024_landmark_bounds.hpp"

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <vector>

namespace sdl3cpp::fs2024 {

float ReadLandmarkRadius(const std::string& outDir,
                        const std::string& model) {
    std::ifstream in(outDir + "/landmarks/" + model + ".lmk",
                    std::ios::binary);
    char magic[4] = {};
    in.read(magic, 4);
    if (!in || std::string(magic, 4) != "LMK1") return 0.f;

    std::uint32_t groups = 0;
    in.read(reinterpret_cast<char*>(&groups), 4);
    float radius = 0.f;
    for (std::uint32_t group = 0; group < groups && in; ++group) {
        std::uint32_t nameLength = 0;
        in.read(reinterpret_cast<char*>(&nameLength), 4);
        in.seekg(nameLength, std::ios::cur);

        std::uint32_t vertexCount = 0;
        in.read(reinterpret_cast<char*>(&vertexCount), 4);
        std::vector<services::impl::BspRenderVertex> vertices(vertexCount);
        in.read(reinterpret_cast<char*>(vertices.data()),
               static_cast<std::streamsize>(
                   vertexCount * sizeof(services::impl::BspRenderVertex)));
        for (const auto& vertex : vertices) {
            radius = std::max(radius, std::hypot(vertex.x, vertex.z));
        }
        std::uint32_t indexCount = 0;
        in.read(reinterpret_cast<char*>(&indexCount), 4);
        in.seekg(static_cast<std::streamoff>(indexCount) * 4, std::ios::cur);
    }
    return radius;
}

}  // namespace sdl3cpp::fs2024
