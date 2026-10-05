#include "racer_model_context.hpp"

namespace sdl3cpp::services::impl::racer_model_detail {
namespace {

constexpr std::uint32_t kVtxBytes = 16;  // N64 Vtx

RacerModelVertex ReadVertex(const MeshCursor& c, std::int32_t index) {
    const RacerBigEndianReader& r = c.walk.reader;
    const std::uint32_t at = c.vertices + kVtxBytes * index;
    const glm::vec4 p = c.transform * glm::vec4(r.I16(at), r.I16(at + 2),
                                                r.I16(at + 4), 1.f);
    RacerModelVertex v;
    v.x = p.x;
    v.y = p.y;
    v.z = p.z;
    // Texture coordinates run 0..1 over the (possibly mirrored) image at
    // 4096 units, as the swe1r-assets exporter reads them.
    v.u = r.I16(at + 8) / 4096.f;
    v.v = r.I16(at + 10) / 4096.f;
    v.r = r.U8(at + 12);
    v.g = r.U8(at + 13);
    v.b = r.U8(at + 14);
    v.a = r.U8(at + 15);
    return v;
}

}  // namespace

void Triangle(MeshCursor& c, std::uint8_t a, std::uint8_t b,
              std::uint8_t d) {
    const std::uint8_t corners[3] = {a, b, d};
    for (std::uint8_t slot : corners) {
        if (slot >= kVertexSlots || c.slots[slot] < 0 ||
            c.slots[slot] >= c.vertexCount) {
            return;  // a slot the list never loaded: skip the triangle
        }
    }
    for (std::uint8_t slot : corners) {
        c.batch->vertices.push_back(ReadVertex(c, c.slots[slot]));
    }
    ++c.walk.model.triangleCount;
}

void LoadVertices(MeshCursor& c, std::uint32_t at) {
    const RacerBigEndianReader& r = c.walk.reader;
    const int n = r.U16(at + 1) >> 4;
    const int v0 = (r.U8(at + 3) >> 1) - n;
    const std::uint32_t address = r.U32(at + 4);
    if (address < c.vertices) return;
    const std::int32_t first = (address - c.vertices) / kVtxBytes;
    for (int k = 0; k < n && v0 + k >= 0 && v0 + k < kVertexSlots; ++k) {
        c.slots[v0 + k] = first + k;
    }
}

}  // namespace sdl3cpp::services::impl::racer_model_detail
