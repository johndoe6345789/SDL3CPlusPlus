#include "services/interfaces/workflow/gta5/gta5_effects.hpp"

#include <cmath>
#include <cstdint>
#include <random>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kCell = 128;  // four across one 512 x 128 strip

void Put(std::vector<std::uint8_t>& px, int x, int y, float white,
         float alpha) {
    std::uint8_t* p = &px[(y * kCell * 4 + x) * 4];
    const auto level = static_cast<std::uint8_t>(255.f * white);
    p[0] = level, p[1] = level, p[2] = level;
    p[3] = static_cast<std::uint8_t>(255.f * std::min(alpha, 1.f));
}

}  // namespace

bool CreateGta5EffectAtlas(Gta5Effects& effects, SDL_GPUDevice* device,
                           Gta5UploadBatch& uploads) {
    std::vector<std::uint8_t> px(kCell * 4 * kCell * 4, 0);
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> noise(0.f, 1.f);
    for (int y = 0; y < kCell; ++y) {
        for (int x = 0; x < kCell; ++x) {
            const float dx = (x + 0.5f) / kCell * 2.f - 1.f;
            const float dy = (y + 0.5f) / kCell * 2.f - 1.f;
            const float r = std::sqrt(dx * dx + dy * dy);
            // A soft round puff: dense in the middle, gone by the edge.
            Put(px, x, y, 1.f, std::pow(std::max(0.f, 1.f - r), 2.f));
            // A flash: a bright core with four spikes.
            const float star =
                std::max(0.f, 1.f - r * 1.4f) +
                std::max(0.f, 0.5f - std::abs(dx) * 6.f) *
                    std::max(0.f, 1.f - std::abs(dy)) +
                std::max(0.f, 0.5f - std::abs(dy) * 6.f) *
                    std::max(0.f, 1.f - std::abs(dx));
            Put(px, x + kCell, y, 1.f, std::min(1.f, star * 1.6f));
            // Smoke: the same ball, mottled, and darker.
            const float lump = 0.6f + 0.4f * noise(rng);
            Put(px, x + 2 * kCell, y, 0.55f,
                std::pow(std::max(0.f, 1.f - r), 1.6f) * lump);
            // A scorch: an uneven dark blotch, hard at the middle.
            const float edge = 0.75f + 0.25f * std::sin(std::atan2(dy, dx) *
                                                        5.f);
            Put(px, x + 3 * kCell, y, 0.04f,
                std::min(1.f, std::pow(std::max(0.f, 1.f - r / edge),
                                       0.45f) * 1.8f));
        }
    }
    effects.atlas =
        CreateGta5MapRgba(device, kCell * 4, kCell, px, uploads);
    SDL_GPUSamplerCreateInfo info = {};
    info.min_filter = SDL_GPU_FILTER_LINEAR;
    info.mag_filter = SDL_GPU_FILTER_LINEAR;
    info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    effects.sampler = SDL_CreateGPUSampler(device, &info);
    effects.ready = effects.atlas && effects.sampler;
    return effects.ready;
}

}  // namespace sdl3cpp::services::impl
