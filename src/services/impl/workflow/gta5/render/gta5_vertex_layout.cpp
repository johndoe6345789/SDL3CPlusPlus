#include "services/interfaces/workflow/gta5/render/gta5_vertex_layout.hpp"

#include "services/interfaces/workflow/gta5/render/gta5_vertex_terrain.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr int kPosition = 0;
constexpr int kNormal = 4;
constexpr int kColour0 = 24;
constexpr int kColour1 = 25;
constexpr int kTexcoord0 = 28;
constexpr int kTexcoord1 = 29;
constexpr std::uint8_t kFloat3 = 6;  // R32G32B32_FLOAT

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
    const auto offset = [&](int slot) { return res.U32(decl + 4 * slot); };
    const auto format = [&](int slot) { return res.U8(decl + 260 + slot); };
    out.position = offset(kPosition);
    out.normal = offset(kNormal);
    out.uv = offset(kTexcoord0);
    out.uv1 = offset(kTexcoord1);
    out.colour0 = offset(kColour0);
    out.colour1 = offset(kColour1);
    out.normalFormat = format(kNormal);
    out.uvFormat = format(kTexcoord0);
    out.uv1Format = format(kTexcoord1);
    out.colour0Format = format(kColour0);
    out.colour1Format = format(kColour1);
    const std::uint64_t end = static_cast<std::uint64_t>(out.data) +
                              std::uint64_t{out.count} * out.stride;
    // Position is the one slot a vertex cannot do without.
    return format(kPosition) == kFloat3 && end <= res.data.size();
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
    // Unflipped: texture rows are stored top first and Vulkan samples v = 0
    // at the first row. 1 - v drew every sign and billboard upside down.
    ReadGta5Uv(res, at + layout.uv, layout.uvFormat, v.u, v.v);
    PackGta5TerrainVertex(res, layout, at, v);
    return v;
}

}  // namespace sdl3cpp::services::impl
