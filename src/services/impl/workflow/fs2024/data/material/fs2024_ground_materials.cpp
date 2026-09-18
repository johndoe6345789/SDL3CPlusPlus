#include "services/interfaces/workflow/fs2024/data/material/fs2024_ground_materials.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace sdl3cpp::fs2024 {
namespace {

constexpr int kCultivated = 1;
constexpr int kForest = 2;
constexpr int kGrassland = 3;
constexpr int kSupportForest = 20;
constexpr int kMeadow = 29;
constexpr int kTemperate = 2;
constexpr std::int64_t kDensityUnit = 1000000000;

/// The integer between `open` and the next '<' after `from`, or -1.
std::int64_t TagValue(const std::string& xml, std::size_t from,
                      const std::string& open) {
    const std::size_t at = xml.find(open, from);
    if (at == std::string::npos) return -1;
    return std::stoll(xml.substr(at + open.size()));
}

}  // namespace

Fs2024GroundMaterials Fs2024GroundMaterials::Read(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("ground materials '" + path + "'");
    std::stringstream buffer;
    buffer << in.rdbuf();
    const std::string xml = buffer.str();

    Fs2024GroundMaterials materials;
    const std::string mark = "<biome land_classifications=\"";
    for (std::size_t at = xml.find(mark); at != std::string::npos;
         at = xml.find(mark, at + mark.size())) {
        const std::int64_t key = std::stoll(xml.substr(at + mark.size()));
        Fs2024GroundMaterial material;
        material.firstLayer =
            static_cast<int>(TagValue(xml, at, "<start_index>"));
        material.layers = static_cast<int>(TagValue(xml, at, "<variations>"));
        if (material.firstLayer >= 0 && material.layers > 0) {
            materials.byKey_.emplace(key, material);
        }
    }
    if (materials.byKey_.empty()) {
        throw std::runtime_error("ground materials: no biomes in " + path);
    }
    return materials;
}

Fs2024GroundMaterial Fs2024GroundMaterials::For(int landClass, int climate,
                                                int density) const {
    if (landClass == kCultivated) landClass = kMeadow;
    if (landClass == kForest) landClass = kSupportForest;
    const std::int64_t plain = climate * 1000 + landClass;
    for (const std::int64_t key :
         {density * kDensityUnit + plain, plain, std::int64_t{landClass}}) {
        const auto it = byKey_.find(key);
        if (it != byKey_.end()) return it->second;
    }
    for (const auto& [key, material] : byKey_) {
        if (key % 1000 == landClass) return material;
    }
    return landClass == kGrassland ? Fs2024GroundMaterial{}
                                   : For(kGrassland, kTemperate);
}

}  // namespace sdl3cpp::fs2024
