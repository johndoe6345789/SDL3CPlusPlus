#include "services/interfaces/workflow/gta5/gta5_vertex_layout.hpp"

#include "services/interfaces/workflow/gta5/gta5_half_float.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr int kPosition = 0;
constexpr int kNormal = 4;
constexpr int kTexcoord0 = 28;
constexpr std::uint8_t kFloat3 = 6;   // R32G32B32_FLOAT
constexpr std::uint8_t kFloat2 = 16;  // R32G32_TYPELESS, read as floats
constexpr std::uint8_t kHalf2 = 34;   // R16G16_FLOAT

}  // namespace

bool ReadGta5VertexLayout(const Gta5Resource& res, std::int64_t buffer,
                          Gta5VertexLayout& out) {
    if (buffer < 0) return false;
    out.count = res.U32(buffer + 0x08);
    out.stride = res.U16(buffer + 0x0C);
    out.data = res.Follow(buffer + 0x18);
    const std::int64_t decl = res.Follow(buffer + 0x38);
    if (decl < 0 || out.data < 0 || out.count == 0 || out.stride == 0) {
        return false;
    }
    out.position = res.U32(decl + 4 * kPosition);
    out.normal = res.U32(decl + 4 * kNormal);
    out.uv = res.U32(decl + 4 * kTexcoord0);
    out.normalFormat = res.U8(decl + 260 + kNormal);
    out.uvFormat = res.U8(decl + 260 + kTexcoord0);
    const std::uint64_t end = static_cast<std::uint64_t>(out.data) +
                              std::uint64_t{out.count} * out.stride;
    // Position is the one slot a vertex cannot do without.
    return res.U8(decl + 260 + kPosition) == kFloat3 &&
           end <= res.data.size();
}

BspRenderVertex ReadGta5Vertex(const Gta5Resource& res,
                               const Gta5VertexLayout& layout,
                               std::uint32_t index) {
    BspRenderVertex v{};
    const std::int64_t at =
        layout.data + std::int64_t{index} * layout.stride;
    const std::int64_t p = at + layout.position;
    v.x = res.F32(p);
    v.y = res.F32(p + 8);
    v.z = -res.F32(p + 4);
    if (layout.normalFormat == kFloat3) {
        const std::int64_t n = at + layout.normal;
        v.nx = res.F32(n);
        v.ny = res.F32(n + 8);
        v.nz = -res.F32(n + 4);
    } else {
        v.ny = 1.f;  // anything is better than a zero normal
    }
    const std::int64_t t = at + layout.uv;
    if (layout.uvFormat == kFloat2) {
        v.u = res.F32(t);
        v.v = 1.f - res.F32(t + 4);
    } else if (layout.uvFormat == kHalf2) {
        v.u = Gta5HalfToFloat(res.U16(t));
        v.v = 1.f - Gta5HalfToFloat(res.U16(t + 2));
    }
    return v;
}

}  // namespace sdl3cpp::services::impl
