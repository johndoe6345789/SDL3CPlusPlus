#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::fs2024 {

/// A BGL angle: 1/2^28 of a semicircle for longitude's 3 semicircles,
/// 2 for latitude's -- see MSFS's own SDK for the format.
double DecodeBglLongitude(std::uint32_t raw);
double DecodeBglLatitude(std::uint32_t raw);

struct Runway {
    int number = 0;
    double lon = 0.0, lat = 0.0;
    float altitude = 0.f;  ///< metres above sea level
    float length = 0.f, width = 0.f;
    float heading = 0.f;  ///< degrees true
};

struct Apron {
    std::vector<std::pair<double, double>> points;  ///< (lon, lat)
    std::vector<std::array<std::uint16_t, 3>> triangles;
};

struct Airport {
    std::string ident;
    double lon = 0.0, lat = 0.0;
    float altitude = 0.f;
    std::vector<Runway> runways;
    std::vector<Apron> aprons;
};

/// Every airport record in `path` that has runways, largest (by apron
/// count) first. Record ids are the ones observed in FS2024's own LOWI
/// package (runway 0xCE, apron 0xD0 inside airport record 0x113);
/// anything else is skipped by its own declared size, so an unknown
/// record type is safe to walk past.
std::vector<Airport> ReadAirports(const std::string& path,
                                  const std::string& ident);

}  // namespace sdl3cpp::fs2024
