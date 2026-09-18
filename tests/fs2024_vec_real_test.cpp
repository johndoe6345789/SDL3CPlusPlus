// FS2024's own vector layer against where things really are: the
// Thames through Westminster and its bridges, and neighbouring tiles
// agreeing where they overlap. Skipped when the game is absent.

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_cgl_container.hpp"
#include "services/interfaces/workflow/fs2024/data/vec/fs2024_vec_frame.hpp"
#include "services/interfaces/workflow/fs2024/data/vec/fs2024_vec_library.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <set>
#include <utility>

namespace f = sdl3cpp::fs2024;

namespace {

constexpr const char* kRoot = "D:/Games/Official/Steam/fs-base-cgl";
constexpr double kPi = 3.14159265358979;

/// (lat, lon) as fractions of `tile` from its north-west corner.
std::pair<double, double> Frac(const f::QuadTile& tile, double lat,
                               double lon) {
    const double side = 1 << tile.level;
    const double s = std::sin(lat * kPi / 180.0);
    const double v = 0.5 - std::log((1 + s) / (1 - s)) / (4 * kPi);
    return {(lon + 180.0) / 360.0 * side - tile.x, v * side - tile.y};
}

std::vector<std::pair<double, double>> Ring(const f::QuadTile& tile,
                                            const f::VecFeature& feature) {
    std::vector<std::pair<double, double>> ring;
    for (const auto& p : feature.points) {
        double x = 0, y = 0;
        f::VecPointInTile(tile, p, x, y);
        ring.push_back({x, y});
    }
    return ring;
}

bool Wet(const f::VecTile& vec, const f::QuadTile& tile, double lat,
         double lon) {
    const auto [x, y] = Frac(tile, lat, lon);
    for (const auto& water : vec.water) {
        const auto ring = Ring(tile, water);
        bool in = false;
        for (std::size_t i = 0, j = ring.size() - 1; i < ring.size();
             j = i++) {
            const auto [xi, yi] = ring[i];
            const auto [xj, yj] = ring[j];
            if ((yi > y) != (yj > y) &&
                x < (xj - xi) * (y - yi) / (yj - yi) + xi) {
                in = !in;
            }
        }
        if (in) return true;
    }
    return false;
}

/// Metres from (lat, lon) to the nearest bridge-flagged road.
double ToBridge(const f::VecTile& vec, const f::QuadTile& tile, double lat,
                double lon, double tileMetres) {
    const auto [bx, by] = Frac(tile, lat, lon);
    double nearest = 1e9;
    for (const auto& road : vec.roads) {
        if (!road.Bridge()) continue;
        const auto line = Ring(tile, road);
        for (std::size_t i = 1; i < line.size(); ++i) {
            const auto [ax, ay] = line[i - 1];
            const auto [cx, cy] = line[i];
            const double dx = cx - ax, dy = cy - ay;
            const double t = std::clamp(
                ((bx - ax) * dx + (by - ay) * dy) /
                    std::max(dx * dx + dy * dy, 1e-12),
                0.0, 1.0);
            nearest = std::min(nearest, std::hypot(ax + t * dx - bx,
                                                   ay + t * dy - by));
        }
    }
    return nearest * tileMetres;
}

}  // namespace

TEST(Fs2024VecReal, WestminsterHasTheThamesWhereItReallyIs) {
    if (!std::filesystem::exists(kRoot)) GTEST_SKIP() << "no FS2024";
    f::VecLibrary library(kRoot);
    const f::QuadTile tile = f::TileAtLatLon(51.5007, -0.1246, 14);
    const f::VecTile vec = library.ReadTile(tile);
    EXPECT_EQ(vec.roads.size(), 2281u);
    EXPECT_EQ(vec.areas.size(), 269u);
    EXPECT_EQ(vec.water.size(), 28u);

    EXPECT_TRUE(Wet(vec, tile, 51.4975, -0.1226));   // off the gardens
    EXPECT_TRUE(Wet(vec, tile, 51.4997, -0.1219));   // below Westminster Br.
    EXPECT_FALSE(Wet(vec, tile, 51.5007, -0.1246));  // Big Ben
    EXPECT_FALSE(Wet(vec, tile, 51.4993, -0.1273));  // Westminster Abbey
    EXPECT_FALSE(Wet(vec, tile, 51.4988, -0.1185));  // St Thomas' Hospital
    EXPECT_FALSE(Wet(vec, tile, 51.4953, -0.1195));  // Lambeth Palace
}

TEST(Fs2024VecReal, BridgesAreTheRealBridges) {
    if (!std::filesystem::exists(kRoot)) GTEST_SKIP() << "no FS2024";
    f::VecLibrary library(kRoot);
    const f::QuadTile tile = f::TileAtLatLon(51.5007, -0.1246, 14);
    const f::VecTile vec = library.ReadTile(tile);
    // Westminster Bridge, and Lambeth Bridge in the southern overlap.
    EXPECT_LT(ToBridge(vec, tile, 51.5008, -0.1218, 1522.6), 30.0);
    EXPECT_LT(ToBridge(vec, tile, 51.4944, -0.1236, 1522.6), 30.0);
    EXPECT_GT(ToBridge(vec, tile, 51.4993, -0.1273, 1522.6), 150.0);
}

TEST(Fs2024VecReal, NeighboursAgreeWhereTheyOverlap) {
    // A road near a tile's east edge is in both tiles: under the true
    // frame the two copies coincide, at London's latitude and at
    // Innsbruck's, where the unit is 7% larger.
    if (!std::filesystem::exists(kRoot)) GTEST_SKIP() << "no FS2024";
    f::VecLibrary library(kRoot);
    for (const auto& [lat, lon] :
         {std::pair{51.5007, -0.1246}, std::pair{47.26, 11.39}}) {
        const f::QuadTile a = f::TileAtLatLon(lat, lon, 14);
        const f::QuadTile b{a.x + 1, a.y, 14};
        std::set<std::pair<long, long>> seen;
        for (const auto& road : library.ReadTile(b).roads) {
            for (const auto& [x, y] : Ring(b, road)) {
                seen.insert({std::lround((x + 1) * 4096),
                             std::lround(y * 4096)});
            }
        }
        int near = 0, both = 0;
        for (const auto& road : library.ReadTile(a).roads) {
            for (const auto& [x, y] : Ring(a, road)) {
                if (x < 0.96 || x > 1.06) continue;
                ++near;
                const long cx = std::lround(x * 4096);
                const long cy = std::lround(y * 4096);
                bool hit = false;
                for (long dx = -1; dx <= 1; ++dx) {
                    for (long dy = -1; dy <= 1; ++dy) {
                        hit = hit || seen.count({cx + dx, cy + dy}) > 0;
                    }
                }
                both += hit;
            }
        }
        ASSERT_GT(near, 20) << lat;
        EXPECT_GT(static_cast<double>(both) / near, 0.8) << lat;
    }
}

TEST(Fs2024VecReal, EveryFinestTileOfLondonsFileDecodes) {
    const std::string path = std::string(kRoot) + "/CGL/031/vec313.cgl";
    if (!std::filesystem::exists(path)) GTEST_SKIP() << "no FS2024";
    const f::CglContainer cgl = f::ReadCglContainer(path);
    std::size_t finest = 0, water = 0;
    for (const auto& entry : cgl.tiles) {
        if ((entry.key >> 16) != 0x8000) continue;  // level 14 only
        ++finest;
        const f::VecTile vec = f::DecodeVecTile(f::ReadCglTile(cgl, entry));
        water += vec.water.size();
    }
    EXPECT_GT(finest, 30000u);
    EXPECT_GT(water, 1000u);
}
