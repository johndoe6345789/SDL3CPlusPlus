#include "services/interfaces/workflow/fs2024/prepare/fs2024_prepare_args.hpp"

#include <stdexcept>

namespace sdl3cpp::tools::fs2024 {
namespace {

const char* kUsage =
    "usage: fs2024_prepare --out DIR --dem FILE.tif "
    "(--icao ICAO [--bgl FILE.bgl] | --lat N --lon N --osm-json FILE.json) "
    "[--tile-size M] [--extent M] [--spacing M] [--texture-per-tile PX] "
    "[--landmark-catalog FILE.json] "
    "[--wall-texture FILE.dds --roof-texture FILE.dds]";

std::string Next(int argc, char** argv, int& i) {
    if (i + 1 >= argc) throw std::runtime_error(kUsage);
    return argv[++i];
}

}  // namespace

PrepareArgs ParsePrepareArgs(int argc, char** argv) {
    PrepareArgs args;
    for (int i = 1; i < argc; ++i) {
        const std::string flag = argv[i];
        if (flag == "--icao") args.icao = Next(argc, argv, i);
        else if (flag == "--bgl") args.bgl = Next(argc, argv, i);
        else if (flag == "--lat") {
            args.lat = std::stod(Next(argc, argv, i));
            args.hasLatLon = true;
        } else if (flag == "--lon") args.lon = std::stod(Next(argc, argv, i));
        else if (flag == "--osm-json") args.osmJson = Next(argc, argv, i);
        else if (flag == "--dem") args.dem = Next(argc, argv, i);
        else if (flag == "--out") args.out = Next(argc, argv, i);
        else if (flag == "--tile-size")
            args.tileSize = std::stof(Next(argc, argv, i));
        else if (flag == "--extent")
            args.extent = std::stof(Next(argc, argv, i));
        else if (flag == "--spacing")
            args.spacing = std::stof(Next(argc, argv, i));
        else if (flag == "--texture-per-tile")
            args.texturePerTile = std::stoi(Next(argc, argv, i));
        else if (flag == "--landmark-catalog")
            args.landmarkCatalog = Next(argc, argv, i);
        else if (flag == "--wall-texture")
            args.wallTexture = Next(argc, argv, i);
        else if (flag == "--roof-texture")
            args.roofTexture = Next(argc, argv, i);
        else throw std::runtime_error(kUsage);
    }
    if (args.dem.empty() || args.out.empty()) throw std::runtime_error(kUsage);
    const bool airportMode = !args.icao.empty();
    if (airportMode == args.hasLatLon) throw std::runtime_error(kUsage);
    if (args.hasLatLon && args.osmJson.empty()) {
        throw std::runtime_error(kUsage);
    }
    return args;
}

}  // namespace sdl3cpp::tools::fs2024
