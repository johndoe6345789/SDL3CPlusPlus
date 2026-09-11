#include "services/interfaces/workflow/gta5/gta5_roads.hpp"

#include "services/interfaces/workflow/gta5/gta5_resource.hpp"

#include <filesystem>
#include <system_error>

namespace sdl3cpp::services::impl {
namespace {

float Fixed(std::uint16_t raw, float scale) {
    return static_cast<float>(static_cast<std::int16_t>(raw)) / scale;
}

/// One file's car nodes; each link's target is left as its (area, id)
/// key in `targets` until every file is in.
void ReadNodes(const Gta5Resource& res, Gta5Roads& roads,
               std::unordered_map<std::uint32_t, std::uint32_t>& index,
               std::vector<std::uint32_t>& targets) {
    const std::int64_t nodes = res.Follow(0x10);
    const std::int64_t links = res.Follow(0x28);
    const std::uint32_t cars = res.U32(0x1C);
    for (std::uint32_t i = 0; i < cars; ++i) {
        const std::int64_t n = nodes + 0x28 * std::int64_t(i);
        // GTA (x, y) in quarter metres and z in 32nds; engine (x, z, -y).
        Gta5RoadNode node;
        node.at = glm::vec3(Fixed(res.U16(n + 0x1C), 4.f),
                            Fixed(res.U16(n + 0x22), 32.f),
                            -Fixed(res.U16(n + 0x1E), 4.f));
        node.firstLink = static_cast<std::uint32_t>(roads.links.size());
        node.linkCount = static_cast<std::uint8_t>(res.U8(n + 0x25) >> 3);
        const std::uint32_t first = res.U16(n + 0x1A);
        for (std::uint32_t k = 0; k < node.linkCount; ++k) {
            const std::int64_t l = links + 8 * std::int64_t(first + k);
            targets.push_back((std::uint32_t(res.U16(l)) << 16) |
                              res.U16(l + 2));
            const std::uint8_t lanes = res.U8(l + 6);
            roads.links.push_back({0, std::uint8_t((lanes >> 5) & 7),
                                   std::uint8_t((lanes >> 2) & 7)});
        }
        const auto self = static_cast<std::uint32_t>(roads.nodes.size());
        index[(std::uint32_t(res.U16(n + 0x10)) << 16) | res.U16(n + 0x12)] =
            self;
        roads.cells[Gta5RoadCell(Gta5RoadCellOf(node.at.x),
                                 Gta5RoadCellOf(node.at.z))]
            .push_back(self);
        roads.nodes.push_back(node);
    }
}

}  // namespace

bool LoadGta5Roads(const std::string& dir, Gta5Roads& roads) {
    std::unordered_map<std::uint32_t, std::uint32_t> index;
    std::vector<std::uint32_t> targets;
    std::error_code error;
    Gta5Resource res;
    std::filesystem::directory_iterator it(dir, error), end;
    for (; !error && it != end; it.increment(error)) {
        if (it->path().extension() != ".ynd") continue;
        if (LoadGta5Resource(it->path().string(), res)) {
            ReadNodes(res, roads, index, targets);
        }
    }
    // Links into foot paths, or areas not loaded, lead nowhere.
    const auto nowhere = static_cast<std::uint32_t>(roads.nodes.size());
    for (std::size_t i = 0; i < roads.links.size(); ++i) {
        const auto found = index.find(targets[i]);
        roads.links[i].to = found == index.end() ? nowhere : found->second;
    }
    roads.loaded = !roads.nodes.empty();
    return roads.loaded;
}

}  // namespace sdl3cpp::services::impl
