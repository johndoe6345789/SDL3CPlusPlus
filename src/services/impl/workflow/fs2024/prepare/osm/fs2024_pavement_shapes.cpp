#include "services/interfaces/workflow/fs2024/prepare/osm/fs2024_pavement_shapes.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::tools::fs2024 {
namespace {

constexpr std::array<std::uint8_t, 3> kAsphalt{52, 52, 55};
constexpr std::array<std::uint8_t, 3> kConcrete{146, 145, 139};

bool IsNarrow(const std::string& highway) {
    return highway == "service" || highway == "footway" ||
          highway == "path" || highway == "pedestrian" ||
          highway == "cycleway" || highway == "steps";
}

}  // namespace

std::vector<Shape> RoadShapes(const std::vector<OsmWay>& roads,
                              const LocalFrame& frame) {
    std::vector<Shape> shapes;
    const float halfWidthDefault = 3.f, halfWidthNarrow = 1.5f;
    for (const OsmWay& road : roads) {
        const float halfWidth =
            IsNarrow(road.type) ? halfWidthNarrow : halfWidthDefault;
        for (std::size_t i = 0; i + 1 < road.points.size(); ++i) {
            float ax, az, bx, bz;
            frame.ToEngine(road.points[i].first, road.points[i].second, ax,
                          az);
            frame.ToEngine(road.points[i + 1].first, road.points[i + 1].second,
                          bx, bz);
            const float dx = bx - ax, dz = bz - az;
            const float length = std::sqrt(dx * dx + dz * dz);
            if (length < 1e-3f) continue;
            const float ux = dx / length, uz = dz / length;
            const float px = -uz * halfWidth, pz = ux * halfWidth;
            const float ex = ux * halfWidth, ez = uz * halfWidth;
            shapes.push_back({kAsphalt,
                             {{ax - ex + px, az - ez + pz},
                              {bx + ex + px, bz + ez + pz},
                              {bx + ex - px, bz + ez - pz},
                              {ax - ex - px, az - ez - pz}}});
        }
    }
    return shapes;
}

std::vector<Shape> AirportShapes(const Airport& airport,
                                 const LocalFrame& frame) {
    std::vector<Shape> shapes;
    for (const Apron& apron : airport.aprons) {
        std::vector<std::pair<float, float>> points;
        points.reserve(apron.points.size());
        for (const auto& [lon, lat] : apron.points) {
            float x, z;
            frame.ToEngine(lon, lat, x, z);
            points.emplace_back(x, z);
        }
        for (const auto& tri : apron.triangles) {
            if (*std::max_element(tri.begin(), tri.end()) >=
                points.size()) {
                continue;
            }
            shapes.push_back(
                {kConcrete, {points[tri[0]], points[tri[1]], points[tri[2]]}});
        }
    }
    if (airport.runways.empty()) return shapes;
    const Runway& runway = *std::max_element(
        airport.runways.begin(), airport.runways.end(),
        [](const Runway& a, const Runway& b) { return a.length < b.length; });

    float cx, cz;
    frame.ToEngine(runway.lon, runway.lat, cx, cz);
    const float h = runway.heading * 3.14159265f / 180.f;
    const float alongX = std::sin(h), alongZ = -std::cos(h);
    const float acrossX = -alongZ, acrossZ = alongX;
    const float halfL = runway.length / 2.f, halfW = runway.width / 2.f;
    std::vector<std::pair<float, float>> rect;
    for (auto [a, c] : {std::pair{-halfL, -halfW}, std::pair{halfL, -halfW},
                       std::pair{halfL, halfW}, std::pair{-halfL, halfW}}) {
        rect.emplace_back(cx + alongX * a + acrossX * c,
                          cz + alongZ * a + acrossZ * c);
    }
    shapes.push_back({kAsphalt, std::move(rect)});
    return shapes;
}

}  // namespace sdl3cpp::tools::fs2024
