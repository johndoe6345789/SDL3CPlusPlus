#include "services/interfaces/workflow/gta5/ped/gta5_ped_frame.hpp"

namespace sdl3cpp::services::impl {

void ReadGta5PedBlend(const Gta5Resource& res, std::int64_t weights,
                      std::int64_t indices, std::int64_t ids,
                      std::uint16_t idCount, Gta5PedPart& part) {
    std::array<std::uint8_t, 4> bones{};
    std::array<float, 4> w{};
    float sum = 0.f;
    for (int k = 0; k < 4; ++k) {
        const std::uint8_t local = res.U8(indices + k);
        bones[k] = static_cast<std::uint8_t>(
            local < idCount ? res.U16(ids + 2 * local) : 0);
        w[k] = res.U8(weights + k) / 255.f;
        sum += w[k];
    }
    for (float& x : w) x = sum > 0.f ? x / sum : 0.f;
    if (sum <= 0.f) w[0] = 1.f;
    part.bones.push_back(bones);
    part.weights.push_back(w);
}

}  // namespace sdl3cpp::services::impl
