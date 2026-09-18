#include "services/interfaces/workflow/fs2024/assemble/fs2024_tile_buildings.hpp"

#include "services/interfaces/workflow/fs2024/assemble/fs2024_building_spot.hpp"
#include "services/interfaces/workflow/fs2024/building/fs2024_oriented_box.hpp"
#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_roof.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr float kGridPerTile = 16384.f;
constexpr float kMetresPerLevel = 3.f;
constexpr std::uint8_t kDefaultLevels = 2;

Fs2024BuildingPlan Plan(const sdl3cpp::fs2024::BldBuilding& source,
                        std::vector<Point2> ring) {
    Fs2024BuildingPlan plan;
    plan.footprint = std::move(ring);
    plan.height = kMetresPerLevel *
                  (source.levels > 0 ? source.levels : kDefaultLevels);
    plan.roof = sdl3cpp::fs2024::RoofShapeOfBld(source.roofType);
    plan.roofSurveyed = source.hasRoofType;
    plan.roofRise = sdl3cpp::fs2024::BldRoofRise(
        plan.roof, ComputeOrientedBox(plan.footprint).halfWidth);
    return plan;
}

}  // namespace

std::vector<Fs2024BuildingPlan> PlanFs2024TileBuildings(
    const std::vector<sdl3cpp::fs2024::BldTile>& tiles, float tileSize) {
    std::vector<Fs2024BuildingPlan> plans;
    const float scale = tileSize / kGridPerTile;
    for (std::size_t set = 0; set < tiles.size(); ++set) {
        for (const auto& source : tiles[set].buildings) {
            if (source.ringSizes.empty() || source.ringSizes[0] < 3) continue;
            std::vector<Point2> ring;
            for (std::uint32_t i = 0; i < source.ringSizes[0]; ++i) {
                // Centre origin, +y north (see BldVertex); tile-local
                // space has its origin at the north-west corner, +y south.
                ring.push_back({tileSize / 2.f + source.vertices[i].x * scale,
                                tileSize / 2.f - source.vertices[i].y * scale});
            }
            const Point2 centre = Fs2024FootprintCentre(ring);
            if (centre.x < 0.f || centre.y < 0.f || centre.x >= tileSize ||
                centre.y >= tileSize) {
                continue;
            }
            Fs2024BuildingPlan* already =
                set == 0 ? nullptr : Fs2024PlanStandingAt(plans, centre);
            if (!already) {
                plans.push_back(Plan(source, std::move(ring)));
            } else if (!already->roofSurveyed && source.hasRoofType) {
                const Fs2024BuildingPlan lent = Plan(source, already->footprint);
                already->roof = lent.roof;
                already->roofRise = lent.roofRise;
                already->roofSurveyed = true;
            }
        }
    }
    return plans;
}

}  // namespace sdl3cpp::services::impl
