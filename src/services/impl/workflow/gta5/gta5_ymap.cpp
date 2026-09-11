#include "services/interfaces/workflow/gta5/gta5_ymap.hpp"

namespace sdl3cpp::services::impl {
namespace {

/// Meta structure hash of CEntityDef. The entities array can also point
/// at a CMloInstanceDef -- a placed interior -- which is laid out
/// differently, so only this one is read.
constexpr std::uint32_t kEntityDefHash = 3461354627u;

struct MetaBlock {
    std::uint32_t hash{0};
    std::int64_t base{-1};
    std::uint32_t length{0};
};

std::vector<MetaBlock> ReadBlocks(const Gta5Resource& res) {
    std::vector<MetaBlock> blocks;
    const std::int64_t table = res.Follow(0x30);
    for (std::uint16_t i = 0; table >= 0 && i < res.U16(0x4C); ++i) {
        const std::int64_t at = table + 16 * static_cast<std::int64_t>(i);
        blocks.push_back({res.U32(at), res.Follow(at + 8), res.U32(at + 4)});
    }
    return blocks;
}

/// A Meta pointer: 1-based block id in the low 12 bits, byte offset in
/// bits 12 to 31. Gives the flat offset, and the block's structure hash.
std::int64_t Deref(const std::vector<MetaBlock>& blocks,
                   std::uint64_t pointer, std::uint32_t* hash = nullptr) {
    const std::uint64_t id = pointer & 0xFFFu;
    const std::uint64_t offset = (pointer >> 12) & 0xFFFFFu;
    if (id == 0 || id > blocks.size()) return -1;
    const MetaBlock& block = blocks[id - 1];
    if (block.base < 0 || offset >= block.length) return -1;
    if (hash) *hash = block.hash;
    return block.base + static_cast<std::int64_t>(offset);
}

Gta5YmapEntity ReadEntity(const Gta5Resource& res, std::int64_t at) {
    Gta5YmapEntity e;
    e.archetype = res.U32(at + 0x08);
    e.position = {res.F32(at + 0x20), res.F32(at + 0x24), res.F32(at + 0x28)};
    e.rotation = {res.F32(at + 0x30), res.F32(at + 0x34), res.F32(at + 0x38),
                  res.F32(at + 0x3C)};
    e.scaleXY = res.F32(at + 0x40);
    e.scaleZ = res.F32(at + 0x44);
    e.lodDist = res.F32(at + 0x4C);
    return e;
}

}  // namespace

std::vector<Gta5YmapEntity> ReadGta5YmapEntities(const Gta5Resource& res) {
    std::vector<Gta5YmapEntity> out;
    const std::vector<MetaBlock> blocks = ReadBlocks(res);
    const std::uint32_t rootId = res.U32(0x1C);
    if (rootId == 0 || rootId > blocks.size()) return out;
    const std::int64_t root = blocks[rootId - 1].base;  // the CMapData
    if (root < 0) return out;

    // CMapData +0x60: the entities, an array of Meta pointers.
    const std::int64_t array = Deref(blocks, res.U64(root + 0x60));
    const std::uint16_t count = res.U16(root + 0x68);
    for (std::uint16_t i = 0; array >= 0 && i < count; ++i) {
        std::uint32_t hash = 0;
        const std::int64_t at = Deref(
            blocks, res.U64(array + 8 * static_cast<std::int64_t>(i)), &hash);
        if (at >= 0 && hash == kEntityDefHash) {
            out.push_back(ReadEntity(res, at));
        }
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
