#include "services/interfaces/workflow/fs2024/prepare/bgl/fs2024_bgl_airport.hpp"

#include "services/interfaces/workflow/fs2024/prepare/fs2024_byte_reader.hpp"

#include <algorithm>
#include <fstream>

namespace sdl3cpp::tools::fs2024 {

namespace {
constexpr std::uint16_t kSectionAirport = 0x03;
constexpr std::uint16_t kRecordAirport = 0x113;
constexpr std::uint16_t kSubRunway = 0xCE;
constexpr std::uint16_t kSubApron = 0xD0;
constexpr std::size_t kAirportHeaderSize = 0x5C;
}  // namespace

double DecodeBglLongitude(std::uint32_t raw) {
    return static_cast<double>(raw) * 360.0 / (3.0 * 268435456.0) - 180.0;
}

double DecodeBglLatitude(std::uint32_t raw) {
    return 90.0 - static_cast<double>(raw) * 180.0 / (2.0 * 268435456.0);
}

namespace {

Runway ReadRunway(const std::vector<std::uint8_t>& data, std::size_t at) {
    Runway runway;
    runway.number = ReadAt<std::uint8_t>(data, at + 8);
    runway.lon = DecodeBglLongitude(ReadAt<std::uint32_t>(data, at + 20));
    runway.lat = DecodeBglLatitude(ReadAt<std::uint32_t>(data, at + 24));
    runway.altitude = ReadAt<std::int32_t>(data, at + 28) / 1000.f;
    runway.length = ReadAt<float>(data, at + 32);
    runway.width = ReadAt<float>(data, at + 36);
    runway.heading = ReadAt<float>(data, at + 40);
    return runway;
}

Apron ReadApron(const std::vector<std::uint8_t>& data, std::size_t at) {
    Apron apron;
    const auto count = ReadAt<std::uint16_t>(data, at + 48);
    const auto triangles = ReadAt<std::uint16_t>(data, at + 50);
    std::size_t p = at + 52;
    for (std::uint16_t i = 0; i < count; ++i, p += 8) {
        apron.points.emplace_back(
            DecodeBglLongitude(ReadAt<std::uint32_t>(data, p)),
            DecodeBglLatitude(ReadAt<std::uint32_t>(data, p + 4)));
    }
    for (std::uint16_t i = 0; i < triangles; ++i, p += 6) {
        apron.triangles.push_back({ReadAt<std::uint16_t>(data, p),
                                   ReadAt<std::uint16_t>(data, p + 2),
                                   ReadAt<std::uint16_t>(data, p + 4)});
    }
    return apron;
}

Airport ReadAirportRecord(const std::vector<std::uint8_t>& data,
                         std::size_t at, const std::string& ident) {
    const auto size = ReadAt<std::uint32_t>(data, at + 2);
    Airport airport;
    airport.ident = ident;
    airport.lon = DecodeBglLongitude(ReadAt<std::uint32_t>(data, at + 12));
    airport.lat = DecodeBglLatitude(ReadAt<std::uint32_t>(data, at + 16));
    airport.altitude = ReadAt<std::int32_t>(data, at + 20) / 1000.f;

    std::size_t sub = at + kAirportHeaderSize;
    while (sub < at + size) {
        const auto subId = ReadAt<std::uint16_t>(data, sub);
        const auto subSize = ReadAt<std::uint32_t>(data, sub + 2);
        if (subSize < 6) break;
        if (subId == kSubRunway) {
            airport.runways.push_back(ReadRunway(data, sub));
        } else if (subId == kSubApron) {
            airport.aprons.push_back(ReadApron(data, sub));
        }
        sub += subSize;
    }
    return airport;
}

}  // namespace

std::vector<Airport> ReadAirports(const std::string& path,
                                  const std::string& ident) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("BGL '" + path + "': cannot open");
    const std::vector<std::uint8_t> data(
        (std::istreambuf_iterator<char>(in)), {});

    std::vector<Airport> found;
    const auto sections = ReadAt<std::uint32_t>(data, 0x14);
    for (std::uint32_t i = 0; i < sections; ++i) {
        const std::size_t sectionAt = 0x38 + i * 20;
        const auto kind = ReadAt<std::uint32_t>(data, sectionAt);
        if (kind != kSectionAirport) continue;
        const auto subs = ReadAt<std::uint32_t>(data, sectionAt + 8);
        const auto tableOffset = ReadAt<std::uint32_t>(data, sectionAt + 12);

        for (std::uint32_t s = 0; s < subs; ++s) {
            const std::size_t subAt = tableOffset + s * 16;
            const auto at = ReadAt<std::uint32_t>(data, subAt + 8);
            const auto length = ReadAt<std::uint32_t>(data, subAt + 12);
            std::size_t record = at;
            while (record < static_cast<std::size_t>(at) + length) {
                const auto recordId = ReadAt<std::uint16_t>(data, record);
                const auto recordSize = ReadAt<std::uint32_t>(data,
                                                              record + 2);
                if (recordId == kRecordAirport) {
                    Airport airport =
                        ReadAirportRecord(data, record, ident);
                    if (!airport.runways.empty()) {
                        found.push_back(std::move(airport));
                    }
                }
                record += recordSize;
            }
        }
    }
    std::sort(found.begin(), found.end(), [](const Airport& a,
                                             const Airport& b) {
        return a.aprons.size() > b.aprons.size();
    });
    return found;
}

}  // namespace sdl3cpp::tools::fs2024
