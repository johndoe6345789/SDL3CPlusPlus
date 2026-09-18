#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_prepare.hpp"

#include "services/interfaces/workflow/fs2024/assemble/fs2024_road_build.hpp"
#include "services/interfaces/workflow/fs2024/assemble/fs2024_sea_build.hpp"
#include "services/interfaces/workflow/fs2024/assemble/fs2024_tile_ground.hpp"
#include "services/interfaces/workflow/fs2024/assemble/fs2024_vegetation_build.hpp"
#include "services/interfaces/workflow/fs2024/landmark/fs2024_tile_landmarks.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_skirt.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kFinestCells = 64;   ///< ~24 m at London, the lcg's grain
constexpr int kCoarseCells = 32;
constexpr int kClassTexels = 64;
constexpr int kBuildingLevels = 2;  ///< the finest two levels get them

}  // namespace

Fs2024PreparedTile PrepareFs2024Tile(Fs2024World& world,
                                     const Fs2024TileKey& key) {
    Fs2024PreparedTile tile;
    tile.key = key;
    const float span = Fs2024TileSpan(key.level, world.origin.TileSize());
    const int cells =
        key.level == kFs2024FinestLevel ? kFinestCells : kCoarseCells;
    int quadX = 0, quadY = 0;
    Fs2024QuadOfKey(world.origin, key, quadX, quadY);

    tile.field = BuildFs2024TileHeights(*world.dem, quadX, quadY, key.level,
                                        span, cells);
    tile.classes = BuildFs2024TileClasses(*world.classes, quadX, quadY,
                                          key.level, kClassTexels);
    tile.classSize = kClassTexels;
    // Water and roads from the vector layer, on the two finest levels.
    Fs2024TileShapes shapes;
    if (world.vectors && key.level > kFs2024FinestLevel - kBuildingLevels) {
        shapes = GatherFs2024TileShapes(*world.vectors, quadX, quadY,
                                        key.level, world.origin.TileSize());
    }
    const Fs2024WaterBuild water = BuildFs2024Water(shapes.water, tile.field);
    tile.water = water.mesh;
    AddFs2024OpenWater(shapes, tile.field, tile.water);
    tile.roads = BuildFs2024Roads(shapes.roads, shapes.water, water.levels,
                                  tile.field);
    tile.ground = BuildFs2024TerrainChunk(tile.field, 0, 0, cells);
    AppendFs2024TerrainSkirt(tile.ground, cells + 1, cells + 1,
                             std::max(20.f, span / 32.f));

    tile.landmarks = PlaceFs2024TileLandmarks(world, key, tile.field);
    const Fs2024LandmarkBoundsMap bounds =
        PrepareFs2024TileLandmarks(world, tile);
    if (key.level > kFs2024FinestLevel - kBuildingLevels) {
        tile.buildings = PrepareFs2024TileBuildings(world, tile, bounds);
    }
    if (world.vegetation &&
        key.level > kFs2024FinestLevel - kBuildingLevels) {
        const glm::vec3 corner = Fs2024TileCorner(key, world.origin.TileSize());
        double lat = 0.0, lon = 0.0;
        Fs2024LatLonOfEngine(world.origin, corner.x + span * 0.5f,
                            corner.z + span * 0.5f, lat, lon);
        const std::uint64_t seed =
            (static_cast<std::uint64_t>(static_cast<std::uint32_t>(quadX))
             << 32) ^
            static_cast<std::uint32_t>(quadY);
        tile.vegetation = BuildFs2024Vegetation(*world.vegetation,
                                                tile.classes, tile.classSize,
                                                tile.field, span, lat, seed);
    }
    return tile;
}

}  // namespace sdl3cpp::services::impl
