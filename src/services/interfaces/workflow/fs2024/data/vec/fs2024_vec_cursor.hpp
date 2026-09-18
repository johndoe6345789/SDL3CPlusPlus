#pragma once

#include "services/interfaces/workflow/fs2024/data/vec/fs2024_vec_tile.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sdl3cpp::fs2024 {

/// The two ways FS2024 writes the same tile layout: the second adds a
/// u16 zero after the areas section and a flags byte to each water
/// record. Nothing in the tile says which; only the wrong one fails.
struct VecLayout {
    bool flaggedWater = false;
};

/// Reads a vector tile's little-endian fields, which are byte aligned
/// but not word aligned. Every read throws when it would run past the
/// blob.
class VecCursor {
public:
    VecCursor(const std::vector<std::uint8_t>& blob, VecLayout layout)
        : blob_(blob), layout_(layout) {}

    std::uint8_t U8();
    std::uint16_t U16();
    std::uint32_t U32();
    void Skip(std::size_t bytes);
    std::size_t Left() const { return blob_.size() - at_; }
    const VecLayout& Layout() const { return layout_; }

    /// A class-mask section header: u32 mask, a u16 cumulative count
    /// per set bit and a u16 zero -- or, for an empty section, the zero
    /// mask alone. Returns each feature's class bit.
    std::vector<int> ClassMask();

    /// `count` points of 4 bytes each.
    std::vector<VecPoint> Points(std::size_t count);

    /// Whether only zero padding -- at most six bytes -- is left.
    bool AtPaddedEnd() const;

private:
    void Need(std::size_t bytes) const;

    const std::vector<std::uint8_t>& blob_;
    VecLayout layout_;
    std::size_t at_ = 0;
};

}  // namespace sdl3cpp::fs2024
