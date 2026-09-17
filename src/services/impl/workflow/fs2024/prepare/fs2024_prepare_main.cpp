// fs2024_prepare: bakes a tile grid for the fs2024 package from a DEM
// GeoTIFF plus either an airport's own BGL, or a saved Overpass JSON
// response for a road/building bake. Makes no network calls of its
// own -- fetch both inputs once with curl, the same boundary gta5
// draws around RPF7 extraction (see packages/fs2024/README.md).

#include "services/interfaces/workflow/fs2024/prepare/bgl/fs2024_bgl_airport.hpp"
#include "services/interfaces/workflow/fs2024/prepare/texture/fs2024_building_kit_extract.hpp"
#include "services/interfaces/workflow/fs2024/prepare/dem/fs2024_dem_tile.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_grid_layout.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_ground_cover.hpp"
#include "services/interfaces/workflow/fs2024/prepare/landmark/fs2024_landmark_catalog.hpp"
#include "services/interfaces/workflow/fs2024/prepare/landmark/fs2024_landmark_extract.hpp"
#include "services/interfaces/workflow/fs2024/prepare/landmark/fs2024_landmark_placement.hpp"
#include "services/interfaces/workflow/fs2024/prepare/landmark/fs2024_landmark_tile_write.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_local_frame.hpp"
#include "services/interfaces/workflow/fs2024/prepare/osm/fs2024_nearest_road.hpp"
#include "services/interfaces/workflow/fs2024/prepare/osm/fs2024_osm_json.hpp"
#include "services/interfaces/workflow/fs2024/prepare/osm/fs2024_pavement_shapes.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_prepare_args.hpp"
#include "services/interfaces/workflow/fs2024/prepare/osm/fs2024_prepare_buildings.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_tile_writer.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>

using namespace sdl3cpp::tools::fs2024;

namespace {

struct Setup {
    LocalFrame frame;
    std::vector<Shape> shapes;
    std::vector<BuildingFootprint> buildings;
    std::vector<OsmWay> rawBuildings;  ///< kept for landmark name-matching
    std::optional<RunwayInfo> runway;
    std::optional<float> flattenTarget;  ///< absolute altitude; road mode
                                        ///< fills this in once sampled
    float spawnX = 0.f, spawnZ = 0.f, spawnHeading = 0.f;
    std::string spawnLabel;
};

Setup SetupAirport(const PrepareArgs& args) {
    const auto airports = ReadAirports(args.bgl, args.icao);
    if (airports.empty()) throw std::runtime_error("no airport records");
    const Airport& airport = airports.front();
    std::printf("%s: %.5f %.5f %.3f m, %zu runways, %zu aprons\n",
               airport.ident.c_str(), airport.lat, airport.lon,
               airport.altitude, airport.runways.size(),
               airport.aprons.size());

    Setup setup{LocalFrame(airport.lon, airport.lat, airport.altitude)};
    setup.shapes = AirportShapes(airport, setup.frame);
    const Runway& runway = *std::max_element(
        airport.runways.begin(), airport.runways.end(),
        [](const Runway& a, const Runway& b) { return a.length < b.length; });
    setup.runway = RunwayInfo{0, 0, runway.heading, runway.length,
                             runway.width, runway.number};
    setup.frame.ToEngine(runway.lon, runway.lat, setup.runway->x,
                        setup.runway->z);
    setup.flattenTarget = runway.altitude;

    float alongX = std::sin(runway.heading * 3.14159265f / 180.f);
    float alongZ = -std::cos(runway.heading * 3.14159265f / 180.f);
    const float back = runway.length / 2.f - 40.f;
    setup.spawnX = setup.runway->x - alongX * back;
    setup.spawnZ = setup.runway->z - alongZ * back;
    setup.spawnHeading = runway.heading;
    setup.spawnLabel = "runway " + std::to_string(runway.number);
    return setup;
}

Setup SetupRoad(const PrepareArgs& args) {
    std::printf("(%.5f, %.5f): reading OpenStreetMap data...\n", args.lat,
               args.lon);
    const OsmData osm = ReadOsmData(args.osmJson);
    const auto nearest = FindNearestRoad(args.lat, args.lon, osm.roads);
    if (!nearest) throw std::runtime_error("no roads found near that point");
    std::printf("  %s (%s), heading %.0f deg\n",
               nearest->road->name.empty() ? nearest->road->type.c_str()
                                           : nearest->road->name.c_str(),
               nearest->road->type.c_str(), nearest->headingDegrees);

    Setup setup{LocalFrame(nearest->lon, nearest->lat, 0.f)};
    setup.shapes = RoadShapes(osm.roads, setup.frame);
    setup.buildings = ConvertBuildings(osm.buildings, setup.frame);
    setup.rawBuildings = osm.buildings;
    setup.spawnHeading = nearest->headingDegrees;
    setup.spawnLabel = nearest->road->name.empty() ? nearest->road->type
                                                   : nearest->road->name;
    return setup;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const PrepareArgs args = ParsePrepareArgs(argc, argv);
        Setup setup = args.icao.empty() ? SetupRoad(args)
                                        : SetupAirport(args);

        const GridLayout grid =
            ComputeGridLayout(args.extent, args.tileSize, args.spacing);
        const int cellsPerTile =
            static_cast<int>(std::lround(grid.tileSize / args.spacing));

        DemTile dem(args.dem);
        std::vector<float> absolute(static_cast<std::size_t>(grid.cells) *
                                    grid.cells);
        for (int row = 0; row < grid.cells; ++row) {
            for (int col = 0; col < grid.cells; ++col) {
                double lon, lat;
                setup.frame.ToGeo(grid.originX + col * args.spacing,
                                 grid.originZ + row * args.spacing, lon, lat);
                absolute[static_cast<std::size_t>(row) * grid.cells + col] =
                    dem.Sample(lon, lat);
            }
        }

        if (!setup.flattenTarget) {
            const int atZero =
                static_cast<int>(std::lround(-grid.originX / args.spacing));
            setup.flattenTarget =
                absolute[static_cast<std::size_t>(atZero) * grid.cells +
                        atZero];
            setup.frame.SetAltitude(*setup.flattenTarget);
        }

        std::vector<std::uint8_t> mask(absolute.size(), 0);
        {
            GroundImage maskImage(grid.cells, grid.cells, grid.originX,
                                 grid.originZ, args.spacing);
            for (const Shape& shape : setup.shapes) {
                maskImage.FillPolygon(shape.polygon, {255, 255, 255});
            }
            for (int row = 0; row < grid.cells; ++row) {
                for (int col = 0; col < grid.cells; ++col) {
                    mask[static_cast<std::size_t>(row) * grid.cells + col] =
                        maskImage.Pixel(col, row)[0];
                }
            }
        }
        FlattenTowards(absolute, grid.cells, mask, *setup.flattenTarget);

        const auto slope =
            ComputeSlopeDegrees(absolute, grid.cells, args.spacing);
        const int tilesPerSide =
            static_cast<int>(std::lround(grid.extent / grid.tileSize));
        const int imageSize = tilesPerSide * args.texturePerTile;
        GroundImage image(imageSize, imageSize, grid.originX, grid.originZ,
                         grid.extent / imageSize);
        for (int y = 0; y < imageSize; ++y) {
            for (int x = 0; x < imageSize; ++x) {
                const int gridCol = std::min(
                    grid.cells - 1,
                    static_cast<int>(static_cast<float>(x) / imageSize *
                                     grid.cells));
                const int gridRow = std::min(
                    grid.cells - 1,
                    static_cast<int>(static_cast<float>(y) / imageSize *
                                     grid.cells));
                const std::size_t at =
                    static_cast<std::size_t>(gridRow) * grid.cells + gridCol;
                image.SetPixel(x, y,
                              CoverColour(absolute[at], slope[at]));
            }
        }
        for (const Shape& shape : setup.shapes) {
            image.FillPolygon(shape.polygon, shape.colour);
        }

        std::vector<float> engineHeights(absolute.size());
        for (std::size_t i = 0; i < absolute.size(); ++i) {
            engineHeights[i] = absolute[i] - setup.frame.altitude();
        }
        const int written = WriteTiles(
            args.out, engineHeights, grid.cells, args.spacing, grid.originX,
            grid.originZ, grid.tileSize, cellsPerTile, image, setup.buildings,
            setup.runway);

        if (!args.wallTexture.empty() && !args.roofTexture.empty()) {
            ExtractBuildingKit(args.wallTexture, args.roofTexture, args.out);
        }

        const auto catalog = ReadLandmarkCatalog(args.landmarkCatalog);
        if (!catalog.empty()) {
            const auto instances =
                MatchLandmarks(setup.rawBuildings, catalog, setup.frame);
            for (const LandmarkCatalogEntry& entry : catalog) {
                const bool matched =
                    std::any_of(instances.begin(), instances.end(),
                               [&](const LandmarkInstance& instance) {
                                   return instance.model == entry.model;
                               });
                if (matched) ExtractLandmarkKit(entry, args.out);
            }
            WriteLandmarkInstances(args.out, instances, grid.tileSize);
            std::printf("%zu of %zu landmarks matched\n", instances.size(),
                       catalog.size());
        }

        const int atZero =
            static_cast<int>(std::lround(-grid.originX / args.spacing));
        const float spawnY =
            setup.runway
                ? *setup.flattenTarget - setup.frame.altitude()
                : absolute[static_cast<std::size_t>(atZero) * grid.cells +
                          atZero] -
                      setup.frame.altitude();

        nlohmann::json world;
        world["lon"] = setup.frame.lon();
        world["lat"] = setup.frame.lat();
        world["altitude"] = setup.frame.altitude();
        world["tile_size"] = grid.tileSize;
        world["spacing"] = args.spacing;
        world["tiles_written"] = written;
        world["spawn"] = {{"x", setup.spawnX},
                          {"z", setup.spawnZ},
                          {"y", spawnY},
                          {"heading", setup.spawnHeading},
                          {"label", setup.spawnLabel}};
        std::ofstream(args.out + "/world.json") << world.dump(2);
        std::printf("%d tiles written; spawn on %s, heading %.0f\n", written,
                   setup.spawnLabel.c_str(), setup.spawnHeading);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "fs2024_prepare: %s\n", error.what());
        return 1;
    }
}
