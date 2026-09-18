#include "services/interfaces/workflow/fs2024/data/bgl/fs2024_scenery_objects.hpp"

#include "services/interfaces/workflow/fs2024/data/bgl/fs2024_bgl_model_name.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>

namespace sdl3cpp::fs2024 {
namespace {

constexpr std::uint32_t kSectionSceneryObject = 0x25;
constexpr std::uint16_t kLibraryObject = 0x000B;
constexpr std::uint16_t kLibraryObjectSize = 64;

template <typename T>
T At(const std::vector<std::uint8_t>& bytes, std::size_t at) {
    T value{};
    if (at + sizeof(T) <= bytes.size()) {
        std::memcpy(&value, &bytes[at], sizeof(T));
    }
    return value;
}

void ReadRecords(const std::vector<std::uint8_t>& bgl, std::size_t from,
                 std::size_t to, std::vector<SceneryObjectPlacement>& out) {
    for (std::size_t at = from; at + 4 <= to && at + 4 <= bgl.size();) {
        const auto id = At<std::uint16_t>(bgl, at);
        const auto size = At<std::uint16_t>(bgl, at + 2);
        if (size < 4) break;  // a zero size would never advance
        if (id == kLibraryObject && size == kLibraryObjectSize &&
            at + size <= bgl.size()) {
            SceneryObjectPlacement p;
            const double lon = At<std::uint32_t>(bgl, at + 4);
            const double lat = At<std::uint32_t>(bgl, at + 8);
            p.lon = lon * 360.0 / 0x30000000 - 180.0;
            p.lat = 90.0 - lat * 180.0 / 0x20000000;
            p.headingDegrees =
                static_cast<float>(At<std::uint16_t>(bgl, at + 0x16) * 360.0 /
                                   65536.0);
            p.guid = HexGuid(&bgl[at + 0x2C]);
            p.scale = At<float>(bgl, at + 0x3C);
            out.push_back(std::move(p));
        }
        at += size;
    }
}

}  // namespace

std::vector<SceneryObjectPlacement> ReadSceneryObjects(
    const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    const std::vector<std::uint8_t> bgl((std::istreambuf_iterator<char>(in)),
                                        std::istreambuf_iterator<char>());
    std::vector<SceneryObjectPlacement> out;
    const auto sections = At<std::uint32_t>(bgl, 0x14);
    for (std::uint32_t i = 0; i < sections; ++i) {
        const std::size_t sectionAt = 0x38 + i * 20ull;
        const auto type = At<std::uint32_t>(bgl, sectionAt);
        if (type != kSectionSceneryObject) continue;
        const auto subs = At<std::uint32_t>(bgl, sectionAt + 8);
        const auto table = At<std::uint32_t>(bgl, sectionAt + 12);
        for (std::uint32_t s = 0; s < subs; ++s) {
            const std::size_t subAt = table + s * 16ull;
            const auto offset = At<std::uint32_t>(bgl, subAt + 8);
            const auto size = At<std::uint32_t>(bgl, subAt + 12);
            ReadRecords(bgl, offset, std::size_t{offset} + size, out);
        }
    }
    return out;
}

}  // namespace sdl3cpp::fs2024
