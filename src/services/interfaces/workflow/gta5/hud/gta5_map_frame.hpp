#pragma once

#include "services/interfaces/workflow/gta5/hud/gta5_map_overlay.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Quads [first, first + count) of a frame, drawn with one texture.
struct Gta5MapRange {
    SDL_GPUTexture* texture{nullptr};
    SDL_GPUSampler* sampler{nullptr};
    std::uint32_t first{0};
    std::uint32_t count{0};
};

/// One frame's overlay: clip-space xyz + uv, six vertices per quad.
struct Gta5MapFrame {
    std::vector<float> vertices;
    std::vector<Gta5MapRange> ranges;
};

/// Where the map sits on screen, in pixels, y down: two tiles wide by
/// three tall, as large as fits with a margin.
struct Gta5MapLayout {
    float width{1.f};
    float height{1.f};
    float left{0.f};
    float top{0.f};
    float tile{1.f};
    /// The screen point at (u, v) in [0, 1] of the map.
    glm::vec2 At(float u, float v) const {
        return {left + u * 2.f * tile, top + v * 3.f * tile};
    }
};

Gta5MapLayout FitGta5Map(int width, int height);

/// A rectangle from `min` to `max` in screen pixels with the uv box (u0,
/// v0, u1, v1). Quads past kGta5MapMaxQuads are dropped.
void AddGta5MapRect(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                    SDL_GPUTexture* texture, SDL_GPUSampler* sampler,
                    glm::vec2 min, glm::vec2 max,
                    glm::vec4 uv = glm::vec4(0.f, 0.f, 1.f, 1.f));

/// A square `half` pixels each way about `centre`, turned `angle`
/// radians clockwise.
void AddGta5MapTurned(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                      SDL_GPUTexture* texture, SDL_GPUSampler* sampler,
                      glm::vec2 centre, float half, float angle);

/// `text` in the map's 5 x 7 lettering, `scale` pixels to a dot, its top
/// left at `at`.
void AddGta5MapText(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                    const Gta5MapOverlay& map, glm::vec2 at, float scale,
                    const std::string& text);

}  // namespace sdl3cpp::services::impl
