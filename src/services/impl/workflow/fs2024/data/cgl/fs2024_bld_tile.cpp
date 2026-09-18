#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_tile.hpp"

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_colour.hpp"
#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_columns.hpp"

#include <numeric>
#include <stdexcept>

namespace sdl3cpp::fs2024 {
namespace {

constexpr std::uint16_t kFlagWallBytes = 0x002;
constexpr std::uint16_t kFlagType = 0x004;
constexpr std::uint16_t kFlagLevels = 0x008;
constexpr std::uint16_t kFlagRoofType = 0x010;
constexpr std::uint16_t kFlagUnnamed40 = 0x040;
constexpr std::uint16_t kFlagUnnamed100 = 0x100;

/// Both coordinate columns: every vertex of the tile in order, each a
/// zig-zag delta on one tile-wide accumulator, x's column whole and
/// then y's.
void ReadCoordinates(BldCursor& cursor, std::vector<BldBuilding>& buildings) {
    std::int32_t running = 0;
    for (BldBuilding& building : buildings) {
        for (BldVertex& vertex : building.vertices) {
            running += cursor.DeltaVarint16();
            vertex.x = running;
        }
    }
    running = 0;
    for (BldBuilding& building : buildings) {
        for (BldVertex& vertex : building.vertices) {
            running += cursor.DeltaVarint16();
            vertex.y = running;
        }
    }
}

}  // namespace

BldTile DecodeBldTile(const std::vector<std::uint8_t>& blob, int version) {
    BldCursor cursor(blob.data(), blob.size());
    const std::uint32_t countA = cursor.Varint16();
    const std::uint32_t countB = version >= 6 ? cursor.Varint16() : 0;
    const std::size_t count = countA + countB;
    SkipBldSubCells(cursor);

    const auto flags = ReadBldFlags(cursor, count, version);
    const auto types = ReadBldByteColumn(cursor, flags, kFlagType);
    const std::vector<std::uint8_t>& typeValues = types.value;
    ReadBldByteColumn(cursor, flags, kFlagUnnamed40);
    if (version >= 6) ReadBldByteColumn(cursor, flags, kFlagUnnamed100);
    const auto roofTypes = ReadBldByteColumn(cursor, flags, kFlagRoofType);
    const auto levels = ReadBldByteColumn(cursor, flags, kFlagLevels);

    BldTile tile;
    tile.buildings.resize(count);
    std::vector<std::uint32_t> vertexCounts(count);
    for (std::size_t i = 0; i < count; ++i) {
        vertexCounts[i] = cursor.Varint8();
        tile.buildings[i].flags = flags[i];
        tile.buildings[i].type = typeValues[i];
        tile.buildings[i].roofType =
            roofTypes.present[i] ? roofTypes.value[i] & 0x1F : 0;
        tile.buildings[i].hasRoofType = roofTypes.present[i];
        tile.buildings[i].levels = levels.present[i] ? levels.value[i] : 0;
        tile.buildings[i].vertices.resize(vertexCounts[i]);
    }
    const auto colourEntries = ReadBldRingsAndPalettes(
        cursor, flags, vertexCounts, typeValues, tile.buildings);
    ReadBldRoofColours(cursor, flags, colourEntries, tile.buildings);
    for (std::size_t i = 0; i < count; ++i) {
        if (flags[i] & kFlagWallBytes) cursor.Skip(vertexCounts[i]);
    }
    ReadCoordinates(cursor, tile.buildings);

    tile.bytesRead = cursor.Position();
    if (!cursor.AtEnd()) throw std::runtime_error("BLD tile: trailing bytes");
    return tile;
}

}  // namespace sdl3cpp::fs2024
