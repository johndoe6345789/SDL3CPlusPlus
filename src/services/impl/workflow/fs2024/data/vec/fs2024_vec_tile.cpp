#include "services/interfaces/workflow/fs2024/data/vec/fs2024_vec_tile.hpp"

#include "services/interfaces/workflow/fs2024/data/vec/fs2024_vec_sections.hpp"

#include <exception>
#include <stdexcept>

namespace sdl3cpp::fs2024 {
namespace {

/// The tile's first three sections in `layout`, true when they and the
/// small table after them parse. The wrong layout fails before that: a
/// water count in the millions, point counts that go backwards, or a
/// missing terminator. (What follows -- rail, and generalised runs with
/// no header at all -- is not read.)
bool Decode(const std::vector<std::uint8_t>& blob, VecLayout layout,
            VecTile& out) {
    try {
        VecCursor in(blob, layout);
        VecTile tile;
        tile.roads = ReadVecSection(in, VecHeader::Mask, VecRecord::Road);
        if (in.Left() > 0) {
            tile.areas = ReadVecSection(in, VecHeader::Mask, VecRecord::Bare);
            if (layout.flaggedWater && in.U16() != 0) return false;
        }
        if (in.Left() > 0) {
            tile.water =
                ReadVecSection(in, VecHeader::Count32, VecRecord::Water);
        }
        if (in.Left() > 0) SkipVecExtra(in);
        out = std::move(tile);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

}  // namespace

VecTile DecodeVecTile(const std::vector<std::uint8_t>& blob) {
    VecTile tile;
    if (blob.size() <= 1) return tile;  // a lone zero: nothing here
    for (const VecLayout layout : {VecLayout{false}, VecLayout{true}}) {
        if (Decode(blob, layout, tile)) return tile;
    }
    throw std::runtime_error("vec: no known layout fits the tile");
}

}  // namespace sdl3cpp::fs2024
