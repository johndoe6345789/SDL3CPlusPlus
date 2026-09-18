// Landmarks stand in their tile where FS2024 places them, each instance
// of a shared model at its own place, and clear the generated buildings
// under them.

#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_clear.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>

namespace f = sdl3cpp::fs2024;
namespace s = sdl3cpp::services::impl;

namespace {

f::LandmarkPlacement At(double lat, double lon, float heading) {
    f::LandmarkPlacement placement;
    placement.model.name = "Pontoon";
    placement.lat = lat;
    placement.lon = lon;
    placement.headingDegrees = heading;
    return placement;
}

s::Fs2024Heightfield FlatGround(float size, float height) {
    s::Fs2024Heightfield field;
    field.columns = field.rows = 2;
    field.spacing = size;
    field.heights.assign(4, height);
    return field;
}

s::Fs2024BuildingPlan Square(float x, float z) {
    s::Fs2024BuildingPlan plan;
    plan.footprint = {{x - 5, z - 5}, {x + 5, z - 5}, {x + 5, z + 5},
                      {x - 5, z + 5}};
    return plan;
}

}  // namespace

TEST(Fs2024TileLandmarks, TwoPlacementsOfOneModelStandApart) {
    s::Fs2024World world;
    world.origin = s::MakeFs2024GeoOrigin(51.5, -0.12);
    s::BucketFs2024Landmarks(
        world, {At(51.4995, -0.1245, 0.f), At(51.4990, -0.1240, 90.f)});
    const auto quad = f::TileAtLatLon(51.4995, -0.1245, 14);
    ASSERT_EQ(world.landmarks.size(), 1u);  // one quad holds both

    const float size = world.origin.TileSize();
    s::Fs2024TileKey key{quad.x - world.origin.tileX,
                         quad.y - world.origin.tileY};
    const glm::vec3 offset(key.x * size, 0.f, key.z * size);
    const auto instances = s::PlaceFs2024TileLandmarks(
        world, quad.x, quad.y, offset, FlatGround(size, 12.f));
    ASSERT_EQ(instances.size(), 2u);
    EXPECT_EQ(instances[0].entry.name, instances[1].entry.name);

    // Each where its own lat/lon puts it, on the ground.
    float x = 0.f, z = 0.f;
    s::Fs2024EngineOfLatLon(world.origin, 51.4990, -0.1240, x, z);
    const glm::vec3 second(instances[1].model[3]);
    EXPECT_NEAR(second.x + offset.x, x, 0.05f);
    EXPECT_NEAR(second.z + offset.z, z, 0.05f);
    EXPECT_NEAR(second.y, 12.f, 1e-3f);
    const glm::vec3 first(instances[0].model[3]);
    EXPECT_GT(glm::distance(first, second), 50.f);
    // Heading 0 faces the model's +z front north (-z); 90 faces east.
    EXPECT_NEAR(glm::vec3(instances[0].model * glm::vec4(0, 0, 1, 0)).z,
                -1.f, 1e-4f);
    EXPECT_NEAR(glm::vec3(instances[1].model * glm::vec4(0, 0, 1, 0)).x,
                1.f, 1e-4f);
}

TEST(Fs2024TileLandmarks, ClearsBuildingsUnderATallLandmarkOnly) {
    s::Fs2024LandmarkInstance palace, pontoon;
    palace.entry.name = "Palace";
    pontoon.entry.name = "Pontoon";
    palace.model = glm::translate(glm::mat4(1.f), glm::vec3(100, 0, 100));
    pontoon.model = glm::translate(glm::mat4(1.f), glm::vec3(400, 0, 400));
    s::Fs2024LandmarkKits kits;
    kits["Palace"].min = glm::vec3(-50, 0, -20);
    kits["Palace"].max = glm::vec3(50, 90, 20);
    kits["Pontoon"].min = glm::vec3(-50, 0, -50);
    kits["Pontoon"].max = glm::vec3(50, 2, 50);

    std::vector<s::Fs2024BuildingPlan> plans = {
        Square(120, 110), Square(100, 150), Square(400, 400)};
    s::DropFs2024BuildingsUnderLandmarks(plans, {palace, pontoon}, kits);
    ASSERT_EQ(plans.size(), 2u);  // only the one inside the palace goes
    EXPECT_FLOAT_EQ(plans[0].footprint[0].y, 145.f);
}
