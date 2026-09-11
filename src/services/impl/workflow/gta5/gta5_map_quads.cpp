#include "services/interfaces/workflow/gta5/gta5_map_frame.hpp"

#include "services/interfaces/workflow/gta5/gta5_map_art.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

glm::vec2 Ndc(const Gta5MapLayout& l, glm::vec2 p) {
    return {p.x / l.width * 2.f - 1.f, 1.f - p.y / l.height * 2.f};
}

/// Corners top left, top right, bottom right, bottom left, in pixels.
void Quad(Gta5MapFrame& f, const Gta5MapLayout& l, SDL_GPUTexture* texture,
          SDL_GPUSampler* sampler, const std::array<glm::vec2, 4>& c,
          glm::vec4 uv) {
    const auto quad = static_cast<std::uint32_t>(f.vertices.size() / 30);
    if (!texture || quad >= kGta5MapMaxQuads) return;
    if (f.ranges.empty() || f.ranges.back().texture != texture ||
        f.ranges.back().sampler != sampler) {
        f.ranges.push_back({texture, sampler, quad, 0});
    }
    ++f.ranges.back().count;
    const glm::vec2 t[4] = {{uv.x, uv.y}, {uv.z, uv.y}, {uv.z, uv.w},
                            {uv.x, uv.w}};
    for (const int i : {0, 1, 2, 0, 2, 3}) {
        const glm::vec2 p = Ndc(l, c[i]);
        f.vertices.insert(f.vertices.end(), {p.x, p.y, 0.f, t[i].x, t[i].y});
    }
}

}  // namespace

Gta5MapLayout FitGta5Map(int width, int height) {
    Gta5MapLayout l;
    l.width = static_cast<float>(std::max(width, 1));
    l.height = static_cast<float>(std::max(height, 1));
    l.tile = std::min(l.width * 0.96f / 2.f, l.height * 0.96f / 3.f);
    l.left = (l.width - 2.f * l.tile) / 2.f;
    l.top = (l.height - 3.f * l.tile) / 2.f;
    return l;
}

void AddGta5MapRect(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                    SDL_GPUTexture* texture, SDL_GPUSampler* sampler,
                    glm::vec2 min, glm::vec2 max, glm::vec4 uv) {
    Quad(frame, layout, texture, sampler,
         {min, glm::vec2(max.x, min.y), max, glm::vec2(min.x, max.y)}, uv);
}

void AddGta5MapTurned(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                      SDL_GPUTexture* texture, SDL_GPUSampler* sampler,
                      glm::vec2 centre, float half, float angle) {
    const float c = std::cos(angle), s = std::sin(angle);
    const auto corner = [&](float dx, float dy) {  // clockwise, y down
        return centre + glm::vec2(dx * c - dy * s, dx * s + dy * c);
    };
    Quad(frame, layout, texture, sampler,
         {corner(-half, -half), corner(half, -half), corner(half, half),
          corner(-half, half)},
         glm::vec4(0.f, 0.f, 1.f, 1.f));
}

void AddGta5MapText(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                    const Gta5MapOverlay& map, glm::vec2 at, float scale,
                    const std::string& text) {
    glm::vec4 uv;
    for (const char ch : text) {
        if (Gta5MapGlyphUv(ch, uv)) {
            AddGta5MapRect(frame, layout, map.atlas, map.nearest, at,
                           at + glm::vec2(5.f, 7.f) * scale, uv);
        }
        at.x += 6.f * scale;
    }
}

}  // namespace sdl3cpp::services::impl
