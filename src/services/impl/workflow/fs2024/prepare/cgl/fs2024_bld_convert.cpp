#include "services/interfaces/workflow/fs2024/prepare/cgl/fs2024_bld_convert.hpp"

#include "services/interfaces/workflow/fs2024/building/fs2024_oriented_box.hpp"
#include "services/interfaces/workflow/fs2024/prepare/cgl/fs2024_bld_merge.hpp"
#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_roof.hpp"

#include <cmath>

namespace sdl3cpp::fs2024 {
namespace {

using sdl3cpp::services::impl::OrientedBox;
using sdl3cpp::services::impl::Point2;
using sdl3cpp::services::impl::RoofShape;

/// FS2024's own storey height: its generator's British styles run
/// three to five levels over the 9-15 m these terraces really stand,
/// so one level is 3 metres.
constexpr float kMetresPerLevel = 3.f;
/// What a building with no surveyed storey count gets. The imagery
/// set carries no levels at all, and FS2024's British distribution
/// rules give its plain houses one to two.
constexpr std::uint8_t kDefaultLevels = 2;

}  // namespace

void AppendBldBuildings(const BldTile& tile, const QuadTile& quad,
                       const LocalFrame& frame, float extent,
                       bool replaceExisting,
                       std::vector<BuildingFootprint>& out) {
    const float half = extent / 2.f;
    for (const BldBuilding& source : tile.buildings) {
        if (source.ringSizes.empty()) continue;
        std::vector<Point2> ring;
        for (std::uint32_t i = 0; i < source.ringSizes[0]; ++i) {
            double lon = 0.0, lat = 0.0;
            BldVertexToLatLon(quad, source.vertices[i].x,
                             source.vertices[i].y, lon, lat);
            Point2 point;
            frame.ToEngine(lon, lat, point.x, point.y);
            ring.push_back(point);
        }
        if (ring.size() < 3) continue;
        const Point2 centre = FootprintCentre(ring);
        if (std::abs(centre.x) > half || std::abs(centre.y) > half) continue;
        const RoofShape shape = RoofShapeOfBld(source.roofType);
        if (!replaceExisting) {
            // The surveyed set knows the outline and the storeys; the
            // imagery set is often the only one that has seen the
            // roof. Where they stand on the same spot, take the roof
            // rather than adding a second building on top of the
            // first -- which is how FS2024 gets a pitched roof onto a
            // footprint whose own record never carried one.
            if (BuildingFootprint* already = BuildingStandingHere(out, centre)) {
                if (!already->roofSurveyed && source.hasRoofType) {
                    already->roof = shape;
                    already->roofRise = BldRoofRise(
                        shape, ComputeOrientedBox(already->footprint)
                                   .halfWidth);
                    already->roofSurveyed = true;
                }
                continue;
            }
        }

        BuildingFootprint building;
        building.footprint = std::move(ring);
        const std::uint8_t levels =
            source.levels > 0 ? source.levels : kDefaultLevels;
        building.height = levels * kMetresPerLevel;
        building.roof = shape;
        building.roofSurveyed = source.hasRoofType;
        const OrientedBox box = ComputeOrientedBox(building.footprint);
        building.roofRise = BldRoofRise(building.roof, box.halfWidth);
        out.push_back(std::move(building));
    }
}

}  // namespace sdl3cpp::fs2024
