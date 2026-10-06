#include "racer_model_context.hpp"

namespace sdl3cpp::services::impl::racer_model_detail {
namespace {

constexpr std::uint32_t kMeshBytes = 0x40;
constexpr std::uint32_t kVtxBytes = 16;     // N64 Vtx
constexpr int kMaxCommands = 8192;

// F3DEX2 opcodes used by the game's display lists.
constexpr std::uint8_t kGVtx = 0x01;
constexpr std::uint8_t kGTri1 = 0x05;
constexpr std::uint8_t kGTri2 = 0x06;
constexpr std::uint8_t kGEndDl = 0xDF;

RacerModelBatch& BatchFor(RacerModel& model, const RacerMaterialRef& m) {
    for (auto& batch : model.batches) {
        if (batch.material == m) return batch;
    }
    model.batches.push_back(RacerModelBatch{m, {}});
    return model.batches.back();
}

}  // namespace

void AppendMesh(ModelWalk& walk, std::uint32_t offset,
                const glm::mat4& transform) {
    const RacerBigEndianReader& r = walk.reader;
    if (!walk.IsPointer(offset, kMeshBytes)) return;
    AppendCollision(walk, offset, transform);
    const std::uint32_t commands = r.U32(offset + 0x30);
    const std::uint32_t vertices = r.U32(offset + 0x34);
    const std::int32_t count = r.I16(offset + 0x3A);
    if (!walk.IsPointer(commands, 8) || count <= 0 ||
        !walk.IsPointer(vertices, kVtxBytes * count)) {
        return;
    }
    const RacerMaterialRef material = ReadMaterial(walk, r.U32(offset));
    MeshCursor c{walk, vertices, count, transform,
                 &BatchFor(walk.model, material), {}};
    c.slots.fill(-1);
    for (int i = 0; i < kMaxCommands; ++i) {
        const std::uint32_t at = commands + 8u * i;
        if (!r.Has(at, 8)) break;
        const std::uint8_t op = r.U8(at);
        if (op == kGEndDl) break;
        if (op == kGVtx) LoadVertices(c, at);
        if (op == kGTri1 || op == kGTri2) {
            Triangle(c, r.U8(at + 1) >> 1, r.U8(at + 2) >> 1,
                     r.U8(at + 3) >> 1);
        }
        if (op == kGTri2) {
            Triangle(c, r.U8(at + 5) >> 1, r.U8(at + 6) >> 1,
                     r.U8(at + 7) >> 1);
        }
    }
    ++walk.model.meshCount;
}

}  // namespace sdl3cpp::services::impl::racer_model_detail
