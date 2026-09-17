#include "services/interfaces/workflow/fs2024/prepare/fs2024_tile_writer.hpp"

#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_key.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>

namespace sdl3cpp::tools::fs2024 {
namespace {

namespace fs = std::filesystem;
using sdl3cpp::services::impl::Fs2024TileDirectory;
using sdl3cpp::services::impl::Point2;
using sdl3cpp::services::impl::Fs2024TileKey;
using sdl3cpp::services::impl::Fs2024TileKeyFor;

void WriteHeightfield(const fs::path& path,
                     const std::vector<float>& heights, int cells,
                     int col0, int row0, int side, float spacing,
                     float tileOriginX, float tileOriginZ) {
    std::ofstream out(path, std::ios::binary);
    out.write("FST1", 4);
    const auto u32Side = static_cast<std::uint32_t>(side);
    out.write(reinterpret_cast<const char*>(&u32Side), 4);
    out.write(reinterpret_cast<const char*>(&u32Side), 4);
    out.write(reinterpret_cast<const char*>(&spacing), 4);
    out.write(reinterpret_cast<const char*>(&tileOriginX), 4);
    out.write(reinterpret_cast<const char*>(&tileOriginZ), 4);
    for (int row = 0; row < side; ++row) {
        const std::size_t at =
            static_cast<std::size_t>(row0 + row) * cells + col0;
        out.write(reinterpret_cast<const char*>(&heights[at]),
                 static_cast<std::streamsize>(side * sizeof(float)));
    }
}

/// AABB overlap between a rotated rectangle (a runway) and a tile's
/// square, both in world metres, `margin` wider on the tile than its
/// own bounds -- a runway routinely spans several tiles.
bool RunwayOverlapsTile(const RunwayInfo& runway, const Fs2024TileKey& key,
                       float tileSize, float margin) {
    const float h = runway.heading * 3.14159265f / 180.f;
    const float alongX = std::sin(h), alongZ = -std::cos(h);
    const float acrossX = -alongZ, acrossZ = alongX;
    const float halfL = runway.length / 2.f, halfW = runway.width / 2.f;
    float minX = 1e30f, minZ = 1e30f, maxX = -1e30f, maxZ = -1e30f;
    for (float a : {-halfL, halfL}) {
        for (float c : {-halfW, halfW}) {
            const float x = runway.x + alongX * a + acrossX * c;
            const float z = runway.z + alongZ * a + acrossZ * c;
            minX = std::min(minX, x); maxX = std::max(maxX, x);
            minZ = std::min(minZ, z); maxZ = std::max(maxZ, z);
        }
    }
    const float tileMinX = key.x * tileSize - margin;
    const float tileMinZ = key.z * tileSize - margin;
    return minX <= tileMinX + tileSize + 2 * margin && maxX >= tileMinX &&
          minZ <= tileMinZ + tileSize + 2 * margin && maxZ >= tileMinZ;
}

void WriteRoadsJson(const fs::path& path,
                   const std::optional<RunwayInfo>& runway) {
    nlohmann::json doc = nlohmann::json::object();
    if (runway) {
        doc["runway"] = {{"x", runway->x},         {"z", runway->z},
                         {"heading", runway->heading},
                         {"length", runway->length}, {"width", runway->width},
                         {"number", runway->number}};
    }
    std::ofstream(path) << doc.dump();
}

/// Whether any of a footprint's points fall within `margin` of a
/// tile's square. Buildings are small relative to a tile, so unlike a
/// runway's rotated-rectangle test, a per-point check is enough.
bool BuildingOverlapsTile(const BuildingFootprint& building,
                         const Fs2024TileKey& key, float tileSize,
                         float margin) {
    const float lowX = key.x * tileSize - margin;
    const float lowZ = key.z * tileSize - margin;
    const float highX = lowX + tileSize + 2 * margin;
    const float highZ = lowZ + tileSize + 2 * margin;
    for (const Point2& p : building.footprint) {
        if (p.x >= lowX && p.x <= highX && p.y >= lowZ && p.y <= highZ) {
            return true;
        }
    }
    return false;
}

void WriteBuildings(const fs::path& path,
                   const std::vector<const BuildingFootprint*>& buildings) {
    std::ofstream out(path, std::ios::binary);
    out.write("FSB1", 4);
    const auto count = static_cast<std::uint32_t>(buildings.size());
    out.write(reinterpret_cast<const char*>(&count), 4);
    for (const BuildingFootprint* b : buildings) {
        const auto points = static_cast<std::uint32_t>(b->footprint.size());
        out.write(reinterpret_cast<const char*>(&points), 4);
        out.write(reinterpret_cast<const char*>(&b->height), 4);
        for (const Point2& p : b->footprint) {
            out.write(reinterpret_cast<const char*>(&p.x), 4);
            out.write(reinterpret_cast<const char*>(&p.y), 4);
        }
    }
}

}  // namespace

int WriteTiles(const std::string& outDir, const std::vector<float>& heights,
              int cells, float spacing, float originX, float originZ,
              float tileSize, int cellsPerTile, const GroundImage& ground,
              const std::vector<BuildingFootprint>& buildings,
              const std::optional<RunwayInfo>& runway) {
    const fs::path tilesDir = fs::path(outDir) / "tiles";
    fs::create_directories(tilesDir);
    const float imgScale =
        static_cast<float>(ground.Width()) /
        (static_cast<float>((cells - 1) / cellsPerTile) * tileSize);

    int written = 0;
    for (int row0 = 0; row0 + cellsPerTile < cells; row0 += cellsPerTile) {
        for (int col0 = 0; col0 + cellsPerTile < cells; col0 += cellsPerTile) {
            const float tileOriginX = originX + col0 * spacing;
            const float tileOriginZ = originZ + row0 * spacing;
            const Fs2024TileKey key =
                Fs2024TileKeyFor(tileOriginX + 1e-2f, tileOriginZ + 1e-2f,
                                 tileSize);
            const fs::path dir = Fs2024TileDirectory(outDir, key);
            fs::create_directories(dir);

            WriteHeightfield(dir / "terrain.fst", heights, cells, col0, row0,
                            cellsPerTile + 1, spacing, tileOriginX,
                            tileOriginZ);

            const int px0 = static_cast<int>((tileOriginX - originX) *
                                             imgScale);
            const int pz0 = static_cast<int>((tileOriginZ - originZ) *
                                             imgScale);
            const int pixels = static_cast<int>(std::round(tileSize *
                                                           imgScale));
            GroundImage tileImage(pixels, pixels, 0.f, 0.f, 1.f);
            for (int y = 0; y < pixels; ++y) {
                for (int x = 0; x < pixels; ++x) {
                    tileImage.SetPixel(x, y, ground.Pixel(px0 + x, pz0 + y));
                }
            }
            tileImage.WritePng((dir / "ground.png").string());

            WriteRoadsJson(dir / "roads.json",
                          runway && RunwayOverlapsTile(*runway, key,
                                                       tileSize, 4.f)
                              ? runway
                              : std::nullopt);

            std::vector<const BuildingFootprint*> here;
            for (const BuildingFootprint& b : buildings) {
                if (BuildingOverlapsTile(b, key, tileSize, 2.f)) {
                    here.push_back(&b);
                }
            }
            WriteBuildings(dir / "buildings.fsb", here);
            ++written;
        }
    }
    return written;
}

}  // namespace sdl3cpp::tools::fs2024
