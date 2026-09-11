#include "services/interfaces/workflow/gta5/gta5_water.hpp"

namespace sdl3cpp::services::impl {
namespace {

BspRenderVertex Corner(float x, float y, float height) {
    BspRenderVertex v{};
    v.x = x, v.y = height, v.z = -y;
    v.u = x / 20.f, v.v = y / 20.f;
    v.ny = 1.f;
    return v;
}

}  // namespace

std::vector<BspRenderVertex> BuildGta5WaterMesh(
    const std::vector<Gta5WaterQuad>& quads) {
    // Which corner a triangle leaves out -- 0 (minX, minY), 1 (maxX, minY),
    // 2 (maxX, maxY), 3 (minX, maxY) -- by type. Fitted against the
    // minimap: across each type's quads one corner is always on the coast.
    constexpr int kDropped[5] = {-1, 0, 3, 2, 1};
    std::vector<BspRenderVertex> out;
    out.reserve(quads.size() * 6);
    for (const Gta5WaterQuad& q : quads) {
        const BspRenderVertex c[4] = {
            Corner(q.minX, q.minY, q.z), Corner(q.maxX, q.minY, q.z),
            Corner(q.maxX, q.maxY, q.z), Corner(q.minX, q.maxY, q.z)};
        const int dropped =
            q.type >= 1 && q.type <= 4 ? kDropped[q.type] : -1;
        if (dropped < 0) {
            out.insert(out.end(), {c[0], c[1], c[2], c[0], c[2], c[3]});
            continue;
        }
        for (int i = 1; i <= 3; ++i) out.push_back(c[(dropped + i) % 4]);
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
