#include "services/interfaces/workflow/gta5/vehicle/gta5_fragment_axles.hpp"

#include <cstdint>

namespace sdl3cpp::services::impl {
namespace {

// wheel_rf, wheel_lf, wheel_rr, wheel_lr
constexpr std::array<std::uint16_t, 4> kAxleBones{26418, 27922, 26398,
                                                  27902};

}  // namespace

bool ReadGta5FragmentAxles(const Gta5Resource& yft,
                           std::array<glm::vec3, 4>& out) {
    const std::int64_t group = yft.Follow(0xF0);
    const std::int64_t lod = group < 0 ? -1 : yft.Follow(group + 0x10);
    if (lod < 0) return false;
    const std::int64_t block = yft.Follow(lod + 0x100);
    const std::int64_t children = yft.Follow(lod + 0xD0);
    if (block < 0 || children < 0) return false;
    const glm::vec3 offset(yft.F32(lod + 0x30), yft.F32(lod + 0x34),
                           yft.F32(lod + 0x38));

    std::array<bool, 4> found{};
    for (std::int64_t i = 0; i < yft.U8(lod + 0x11D); ++i) {
        const std::int64_t child = yft.Follow(children + 8 * i);
        if (child < 0) continue;
        const std::uint16_t tag = yft.U16(child + 0x12);
        const std::int64_t row = block + 0x20 + 64 * i + 48;
        const glm::vec3 g = glm::vec3(yft.F32(row), yft.F32(row + 4),
                                      yft.F32(row + 8)) +
                            offset;
        for (std::size_t w = 0; w < kAxleBones.size(); ++w) {
            if (tag != kAxleBones[w]) continue;
            // GTA to engine is (x, z, -y), and turning to face +z negates
            // x and z: (-x, z, y) overall.
            out[w] = glm::vec3(-g.x, g.z, g.y);
            found[w] = true;
        }
    }
    return found[0] && found[1] && found[2] && found[3];
}

}  // namespace sdl3cpp::services::impl
