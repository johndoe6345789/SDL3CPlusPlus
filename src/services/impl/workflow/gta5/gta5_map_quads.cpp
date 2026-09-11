#include "services/interfaces/workflow/gta5/gta5_map_overlay.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

void Push(std::vector<float>& out, glm::vec2 p, float u, float v) {
    out.insert(out.end(), {p.x, p.y, 0.f, u, v});
}

void Quad(std::vector<float>& out, glm::vec2 tl, glm::vec2 tr, glm::vec2 br,
          glm::vec2 bl) {
    Push(out, tl, 0.f, 0.f), Push(out, tr, 1.f, 0.f), Push(out, br, 1.f, 1.f);
    Push(out, tl, 0.f, 0.f), Push(out, br, 1.f, 1.f), Push(out, bl, 0.f, 1.f);
}

}  // namespace

std::vector<float> BuildGta5MapQuads(int width, int height, float u, float v,
                                     float angle) {
    std::vector<float> out;
    out.reserve(kGta5MapQuads * 30);
    Quad(out, {-1.f, 1.f}, {1.f, 1.f}, {1.f, -1.f}, {-1.f, -1.f});
    const float w = static_cast<float>(std::max(width, 1));
    const float h = static_cast<float>(std::max(height, 1));
    // Two tiles wide by three tall, as large as fits with a margin.
    const float tile = std::min(w * 0.96f / 2.f, h * 0.96f / 3.f);
    const float left = (w - 2.f * tile) / 2.f, top = (h - 3.f * tile) / 2.f;
    const auto ndc = [&](float x, float y) {
        return glm::vec2(x / w * 2.f - 1.f, 1.f - y / h * 2.f);
    };
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 2; ++column) {
            const float x = left + column * tile, y = top + row * tile;
            Quad(out, ndc(x, y), ndc(x + tile, y), ndc(x + tile, y + tile),
                 ndc(x, y + tile));
        }
    }
    const float mx = left + std::clamp(u, 0.f, 1.f) * 2.f * tile;
    const float my = top + std::clamp(v, 0.f, 1.f) * 3.f * tile;
    const float c = std::cos(angle), s = std::sin(angle), r = 14.f;
    const auto corner = [&](float dx, float dy) {  // clockwise, y down
        return ndc(mx + dx * c - dy * s, my + dx * s + dy * c);
    };
    Quad(out, corner(-r, -r), corner(r, -r), corner(r, r), corner(-r, r));
    return out;
}

}  // namespace sdl3cpp::services::impl
