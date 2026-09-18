// FS2024's own world DEM against the installed game and against real
// terrain. Skipped when either the game or the Copernicus reference
// tiles are absent -- there is nothing to substitute for either.

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_cgl_container.hpp"
#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_quadkey.hpp"
#include "services/interfaces/workflow/fs2024/data/dem/fs2024_dem_calibration.hpp"
#include "services/interfaces/workflow/fs2024/data/dem/fs2024_dem_cgl_tile.hpp"
#include "services/interfaces/workflow/fs2024/data/dem/fs2024_dem_tile.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <map>
#include <string>

namespace f = sdl3cpp::fs2024;

namespace {

constexpr const char* kCgl = "D:/Games/Official/Steam/fs-base-cgl/CGL/";
constexpr int kDemLevel = 10;  // the finest level a dem file carries

/// The level-10 tile over (lat, lon), decoded, and its quad position.
bool LoadDem(double lat, double lon, f::QuadTile& quad,
             f::Fs2024DemSamples& out) {
    quad = f::TileAtLatLon(lat, lon, kDemLevel);
    const std::string key = f::QuadKeyOf(quad);
    const std::string path = std::string(kCgl) + key.substr(0, 3) + "/dem" +
                             key.substr(3, 3) + ".cgl";
    if (!std::filesystem::exists(path)) return false;
    const auto cgl = f::ReadCglContainer(path);
    std::uint32_t index = 0;
    for (std::size_t i = 6; i < key.size(); ++i) index = index * 4 + (key[i] - '0');
    const std::uint32_t inFile = ((kDemLevel - 6) << 12) | index;
    const f::CglTileEntry* entry = f::FindCglTile(cgl, inFile);
    if (!entry) return false;
    out = f::DecodeFs2024DemTile(f::ReadCglTile(cgl, *entry));
    return true;
}

/// Lon/lat of sample (column, row): a raster from the tile's north-west
/// corner, 256 intervals across -- not the building layer's convention.
void SampleLonLat(const f::QuadTile& quad, int column, int row, double& lon,
                  double& lat) {
    const double side = 1 << quad.level;
    const double u = (quad.x + column / 256.0) / side;
    const double v = (quad.y + row / 256.0) / side;
    lon = u * 360.0 - 180.0;
    lat = std::atan(std::sinh(3.14159265358979 * (1.0 - 2.0 * v))) *
          180.0 / 3.14159265358979;
}

}  // namespace

TEST(Fs2024DemReal, LondonTileIs257SquareBd16s) {
    f::QuadTile quad;
    f::Fs2024DemSamples dem;
    if (!LoadDem(51.5007, -0.1246, quad, dem)) GTEST_SKIP() << "no FS2024";
    EXPECT_EQ(dem.width, 257);
    EXPECT_EQ(dem.height, 257);
}

namespace {

/// The commonest calibrated height in a 33 x 33 window around a point:
/// a lake is flat, so that is its surface.
float SurfaceNear(const f::QuadTile& quad, const f::Fs2024DemSamples& dem,
                  double lat, double lon) {
    const double side = 1 << kDemLevel;
    const double u = (lon + 180.0) / 360.0 * side - quad.x;
    const double s = std::sin(lat * 3.14159265358979 / 180.0);
    const double v =
        (0.5 - std::log((1 + s) / (1 - s)) / (4 * 3.14159265358979)) * side -
        quad.y;
    const int c0 = static_cast<int>(u * 256), r0 = static_cast<int>(v * 256);
    std::map<int, int> counts;
    for (int r = std::max(0, r0 - 16); r <= std::min(256, r0 + 16); ++r) {
        for (int c = std::max(0, c0 - 16); c <= std::min(256, c0 + 16); ++c) {
            ++counts[dem.At(c, r)];
        }
    }
    int best = 0, mode = 0;
    for (const auto& [raw, n] : counts) {
        if (n > best) { best = n; mode = raw; }
    }
    return f::Fs2024DemMetres(static_cast<std::int16_t>(mode));
}

}  // namespace

TEST(Fs2024DemReal, LakeSurfacesMatchTheirSurveyedHeights) {
    // Not used to build the calibration: an out-of-sample check that
    // FS2024's own tiles put known water at its known height.
    struct Lake { const char* name; double lat, lon, metres; };
    const Lake lakes[] = {
        {"Dead Sea", 31.55, 35.47, -430}, {"Salton Sea", 33.30, -115.80, -69},
        {"Caspian", 42.00, 51.00, -28},   {"Geneva", 46.45, 6.55, 372},
        {"Victoria", -1.00, 33.00, 1135}, {"Tahoe", 39.09, -120.04, 1897},
        {"Uyuni", -20.13, -67.49, 3656},  {"Titicaca", -15.80, -69.40, 3812},
        {"Nam Co", 30.70, 90.60, 4718}};
    f::QuadTile quad;
    f::Fs2024DemSamples dem;
    if (!LoadDem(lakes[0].lat, lakes[0].lon, quad, dem)) {
        GTEST_SKIP() << "no FS2024";
    }
    for (const Lake& lake : lakes) {
        ASSERT_TRUE(LoadDem(lake.lat, lake.lon, quad, dem)) << lake.name;
        EXPECT_NEAR(SurfaceNear(quad, dem, lake.lat, lake.lon), lake.metres,
                    70.0) << lake.name;
    }
}

TEST(Fs2024DemReal, MatchesCopernicusTerrain) {
    // Copernicus GLO-30 is a surface model at 30 m; FS2024's is ~95 m
    // here, so the tolerances grow with relief. Skips without the tiles.
    struct Region { const char* name; double lat, lon; const char* tif;
                    double maxRms; };
    const std::string dir = "D:/fs2024/dem/Copernicus_DSM_COG_10_";
    const Region regions[] = {
        {"London", 51.50, -0.25, "N51_00_W001_00", 6.0},
        {"Innsbruck", 47.25, 11.40, "N47_00_E011_00", 40.0},
        {"Dead Sea", 31.50, 35.50, "N31_00_E035_00", 35.0},
        {"Everest", 27.99, 86.93, "N27_00_E086_00", 75.0}};
    for (const Region& region : regions) {
        const std::string tif = dir + region.tif + "_DEM.tif";
        f::QuadTile quad;
        f::Fs2024DemSamples dem;
        if (!std::filesystem::exists(tif) ||
            !LoadDem(region.lat, region.lon, quad, dem)) {
            GTEST_SKIP() << "no reference for " << region.name;
        }
        const f::DemTile reference(tif);
        double sum = 0.0;
        int n = 0;
        for (int row = 4; row < dem.height - 4; row += 4) {
            for (int column = 4; column < dem.width - 4; column += 4) {
                double lon, lat;
                SampleLonLat(quad, column, row, lon, lat);
                double truth = 0.0;
                try { truth = reference.Sample(lon, lat); } catch (...) { continue; }
                const double e = f::Fs2024DemMetres(dem.At(column, row)) - truth;
                sum += e * e;
                ++n;
            }
        }
        ASSERT_GT(n, 100) << region.name;
        EXPECT_LT(std::sqrt(sum / n), region.maxRms) << region.name;
    }
}
