#include "services/interfaces/workflow/gta5/render/gta5_vertex_terrain.hpp"

#include "services/interfaces/workflow/gta5/render/gta5_half_float.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr std::uint8_t kFloat2 = 16;  // R32G32_TYPELESS, read as floats
constexpr std::uint8_t kUnorm4 = 28;  // R8G8B8A8_UNORM
constexpr std::uint8_t kHalf2 = 34;   // R16G16_FLOAT

/// A coordinate in [0, 1] as a whole number of 4096ths.
float Twelve(float x) { return std::round(std::clamp(x, 0.f, 1.f) * 4095.f); }

}  // namespace

bool ReadGta5Uv(const Gta5Resource& res, std::int64_t at,
                std::uint8_t format, float& u, float& v) {
    if (format == kFloat2) {
        u = res.F32(at);
        v = res.F32(at + 4);
        return true;
    }
    if (format == kHalf2) {
        u = Gta5HalfToFloat(res.U16(at));
        v = Gta5HalfToFloat(res.U16(at + 2));
        return true;
    }
    return false;
}

void PackGta5TerrainVertex(const Gta5Resource& res,
                           const Gta5VertexLayout& layout, std::int64_t at,
                           BspRenderVertex& v) {
    // R8G8B8A8: red at +0, green +1, blue +2, alpha +3.
    const bool has1 = layout.colour1Format == kUnorm4;
    const std::int64_t c1 = at + layout.colour1;
    const float blue = has1 ? res.U8(c1 + 2) : 128.f;
    const float green = has1 ? res.U8(c1 + 1) : 128.f;
    float alpha = 0.f;
    if (has1) {
        alpha = layout.colour0Format == kUnorm4
                    ? static_cast<float>(res.U8(at + layout.colour0 + 3))
                    : 255.f;
    }
    v.lm_u = blue + 256.f * green + 65536.f * alpha;
    float u1 = 0.f, v1 = 0.f;
    ReadGta5Uv(res, at + layout.uv1, layout.uv1Format, u1, v1);
    v.lm_v = Twelve(u1) + 4096.f * Twelve(v1);
}

}  // namespace sdl3cpp::services::impl
