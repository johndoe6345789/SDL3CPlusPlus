#include "services/interfaces/workflow/gta5/hud/gta5_map_frame.hpp"

#include "services/interfaces/workflow/gta5/hud/gta5_map_art.hpp"

namespace sdl3cpp::services::impl {
namespace {

glm::vec2 Ndc(const Gta5MapLayout& l, glm::vec2 p) {
    return {p.x / l.width * 2.f - 1.f, 1.f - p.y / l.height * 2.f};
}

}  // namespace

void AddGta5MapQuad(Gta5MapFrame& f, const Gta5MapLayout& l,
                    SDL_GPUTexture* texture, SDL_GPUSampler* sampler,
                    const std::array<glm::vec2, 4>& c, glm::vec4 uv) {
    const auto quad = static_cast<std::uint32_t>(f.vertices.size() / 30);
    if (!texture || quad >= kGta5MapMaxQuads) return;
    if (f.ranges.empty() || f.ranges.back().texture != texture ||
        f.ranges.back().sampler != sampler ||
        f.ranges.back().clip != f.clip) {
        f.ranges.push_back({texture, sampler, quad, 0, f.clip});
    }
    ++f.ranges.back().count;
    const glm::vec2 t[4] = {
        {uv.x, uv.y}, {uv.z, uv.y}, {uv.z, uv.w}, {uv.x, uv.w}};
    for (const int i : {0, 1, 2, 0, 2, 3}) {
        const glm::vec2 p = Ndc(l, c[i]);
        f.vertices.insert(f.vertices.end(),
                          {p.x, p.y, 0.f, t[i].x, t[i].y});
    }
}

}  // namespace sdl3cpp::services::impl
