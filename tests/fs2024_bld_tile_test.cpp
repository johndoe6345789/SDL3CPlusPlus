// DecodeBldTile against a tile packed by hand exactly as FS2024's
// own building data is: column-major, every attribute a running sum
// mod 256 that only advances for the buildings carrying its flag,
// the flag word itself split across nine bitplanes, and coordinates
// as one zig-zag delta run for the whole tile. The zig-zag is NOT
// protobuf's: odd is negative, so `7` decodes to -3, not -4.

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_tile.hpp"

#include <gtest/gtest.h>

namespace tools = sdl3cpp::fs2024;

namespace {

struct BldWriter {
    std::vector<std::uint8_t> bytes;
    void U8(int value) { bytes.push_back(static_cast<std::uint8_t>(value)); }
    void U16(int value) { U8(value & 0xFF); U8((value >> 8) & 0xFF); }
    void Varint8(int value) { U8(value); }        // short form only
    void Varint16(int value) { U16(value); }      // short form only
    void Delta(int value) {
        Varint16(value >= 0 ? value * 2 : -value * 2 + 1);
    }
};

/// Two buildings: a four-sided one with type/levels/roof/colour, and
/// an eight-vertex one with a hole and no attributes at all.
std::vector<std::uint8_t> TwoBuildingTile() {
    BldWriter w;
    w.Varint16(2);        // count_a
    w.Varint16(0);        // count_b
    w.U8(0);              // no sub-cell entries
    w.U16(0x3D);          // planes for bits 0, 2, 3, 4, 5
    w.U8(0b10);           // bit 0 (rings): building 1
    w.U8(0b01);           // bit 2 (type): building 0
    w.U8(0b01);           // bit 3 (levels)
    w.U8(0b01);           // bit 4 (roof type)
    w.U8(0b01);           // bit 5 (roof colour)
    w.U8('M');            // type column
    w.U8(3);              // roof type column
    w.U8(4);              // levels column
    w.Varint8(4);         // vertex counts
    w.Varint8(8);
    w.U8(1);              // building 1: one hole ring
    w.U8(3);              //   of three vertices
    w.U8(20); w.U8(21); w.U8(20);  // red, green, blue columns
    const int xs[] = {100, 10, -20, -10, 500, 5, 5, -5, -5, 1, 1, -1};
    const int ys[] = {-50, 5, 5, -5, 200, 10, -10, -10, 10, 2, -2, 2};
    for (int value : xs) w.Delta(value);
    for (int value : ys) w.Delta(value);
    return w.bytes;
}

}  // namespace

TEST(Fs2024BldTile, DecodesAttributesRingsAndAbsoluteVertices) {
    const tools::BldTile tile = tools::DecodeBldTile(TwoBuildingTile(), 6);
    ASSERT_EQ(tile.buildings.size(), 2u);
    const tools::BldBuilding& first = tile.buildings[0];
    EXPECT_EQ(first.type, 'M');
    EXPECT_EQ(first.roofType, 3);
    EXPECT_EQ(first.levels, 4);
    EXPECT_TRUE(first.hasRoofColour);
    EXPECT_EQ(first.red, 20);
    EXPECT_EQ(first.green, 21);
    EXPECT_EQ(first.blue, 20);
    ASSERT_EQ(first.vertices.size(), 4u);
    EXPECT_EQ(first.vertices[0].x, 100);
    EXPECT_EQ(first.vertices[1].x, 110);   // deltas accumulate
    EXPECT_EQ(first.vertices[2].x, 90);    // 7 -> -20, not -21
    EXPECT_EQ(first.vertices[0].y, -50);

    const tools::BldBuilding& second = tile.buildings[1];
    EXPECT_EQ(second.type, 0);     // no type flag of its own, so none
    EXPECT_EQ(second.levels, 0);   // unsurveyed, not its neighbour's 4
    EXPECT_FALSE(second.hasRoofColour);
    ASSERT_EQ(second.ringSizes.size(), 2u);
    EXPECT_EQ(second.ringSizes[0], 5u);    // outer = 8 - 3
    EXPECT_EQ(second.ringSizes[1], 3u);    // the hole
    ASSERT_EQ(second.vertices.size(), 8u);
    EXPECT_EQ(second.vertices[0].x, 580);  // x runs on across buildings
    EXPECT_EQ(tile.bytesRead, TwoBuildingTile().size());
}

TEST(Fs2024BldTile, TrailingBytesAreRejected) {
    std::vector<std::uint8_t> blob = TwoBuildingTile();
    blob.push_back(0);
    EXPECT_THROW(tools::DecodeBldTile(blob, 6), std::runtime_error);
}
