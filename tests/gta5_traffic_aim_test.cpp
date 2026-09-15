// Where a traffic car steers, pinned.
//
// Gta5TrafficAim reads roads.nodes[car.from] and roads.nodes[car.to]
// to build its lane point, and the bounds check for `to` used to sit
// after that read -- so a link off the end of the loaded network was
// indexed before anything rejected it. The first two cases here are
// that guard. The rest hold the shape of the steering itself: the lane
// is on the right of the way the road runs, a car far from its node
// aims straight at it, and one nearing a corner leans onto the road
// after it rather than sawing at the node.

#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_path.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kLane = 2.2f;

/// A node at `at` linked to each of `to`, appended to `roads`.
void AddNode(Gta5Roads& roads, const glm::vec3& at,
             const std::vector<std::uint32_t>& to) {
    Gta5RoadNode node;
    node.at = at;
    node.firstLink = static_cast<std::uint32_t>(roads.links.size());
    node.linkCount = static_cast<std::uint8_t>(to.size());
    for (const std::uint32_t link : to) {
        roads.links.push_back(Gta5RoadLink{link, 1u, 1u});
    }
    roads.nodes.push_back(node);
    roads.loaded = true;
}

Gta5TrafficCar Car(std::uint32_t from, std::uint32_t to) {
    Gta5TrafficCar car;
    car.from = from;
    car.to = to;
    car.lane = kLane;
    return car;
}

/// Node 0 at the origin, node 1 a hundred metres up +z, linked.
Gta5Roads Straight() {
    Gta5Roads roads;
    AddNode(roads, glm::vec3(0.f, 0.f, 0.f), {1u});
    AddNode(roads, glm::vec3(0.f, 0.f, 100.f), {0u});
    return roads;
}

/// The same, turning right at node 1 towards node 2 out along +x.
Gta5Roads RightTurn() {
    Gta5Roads roads;
    AddNode(roads, glm::vec3(0.f, 0.f, 0.f), {1u});
    AddNode(roads, glm::vec3(0.f, 0.f, 100.f), {0u, 2u});
    AddNode(roads, glm::vec3(100.f, 0.f, 100.f), {1u});
    return roads;
}

void ExpectVec(const glm::vec3& got, const glm::vec3& want) {
    EXPECT_NEAR(got.x, want.x, 1e-3f);
    EXPECT_NEAR(got.y, want.y, 1e-3f);
    EXPECT_NEAR(got.z, want.z, 1e-3f);
}

// An empty network must be rejected before either node is read.
TEST(Gta5TrafficAim, EmptyNetworkAimsWhereItStands) {
    const Gta5Roads roads;
    const glm::vec3 at(5.f, 0.f, 7.f);
    ExpectVec(Gta5TrafficAim(roads, Car(0u, 0u), at, 10.f), at);
}

// And so must a link that leads off the end of what is loaded.
TEST(Gta5TrafficAim, LinkPastTheEndAimsWhereItStands) {
    const Gta5Roads roads = Straight();
    const glm::vec3 at(2.2f, 0.f, 10.f);
    ExpectVec(Gta5TrafficAim(roads, Car(0u, 9u), at, 10.f), at);
    ExpectVec(Gta5TrafficAim(roads, Car(9u, 1u), at, 10.f), at);
}

// Well short of the node, the aim is the node held into the lane.
TEST(Gta5TrafficAim, FarFromTheNodeAimsAtTheLanePoint) {
    const Gta5Roads roads = Straight();
    const glm::vec3 at(kLane, 0.f, 0.f);
    ExpectVec(Gta5TrafficAim(roads, Car(0u, 1u), at, 10.f),
              glm::vec3(kLane, 0.f, 100.f));
}

// The hold-over is to the right of the way the road runs, so the two
// directions pass rather than meet: +x going up +z, -x coming back.
TEST(Gta5TrafficAim, TheLaneIsOnTheRightOfTheWayItRuns) {
    const Gta5Roads roads = Straight();
    const glm::vec3 up = Gta5TrafficAim(roads, Car(0u, 1u),
                                        glm::vec3(0.f, 0.f, 0.f), 10.f);
    EXPECT_NEAR(up.x, kLane, 1e-3f);
    const glm::vec3 back = Gta5TrafficAim(roads, Car(1u, 0u),
                                          glm::vec3(0.f, 0.f, 100.f), 10.f);
    EXPECT_NEAR(back.x, -kLane, 1e-3f);
}

// Nearing the corner, the aim slides off the node and onto the road
// after it -- the whole point of looking ahead.
TEST(Gta5TrafficAim, NearingACornerLeansOntoTheNextRoad) {
    const Gta5Roads roads = RightTurn();
    // Four metres short of the node, looking seven ahead.
    const glm::vec3 aim = Gta5TrafficAim(roads, Car(0u, 1u),
                                         glm::vec3(kLane, 0.f, 96.f), 0.f);
    EXPECT_GT(aim.x, kLane + 1.f);   // off the node, into the turn
    EXPECT_LT(aim.x, 100.f);         // but not all the way onto it
    EXPECT_LT(aim.z, 100.f);         // and drawn back off the corner
}

// At exactly the look distance the lean is zero, so the aim does not
// jump as a car crosses it.
TEST(Gta5TrafficAim, AtTheLookDistanceTheAimIsStillTheNode) {
    const Gta5Roads roads = RightTurn();
    const glm::vec3 node(kLane, 0.f, 100.f);
    // Look is clamped to seven metres at a standstill; sit on it.
    ExpectVec(Gta5TrafficAim(roads, Car(0u, 1u),
                             glm::vec3(kLane, 0.f, 93.f), 0.f), node);
    ExpectVec(Gta5TrafficAim(roads, Car(0u, 1u),
                             glm::vec3(kLane, 0.f, 92.9f), 0.f), node);
}

// The road back is not a choice while any other one exists, or the
// traffic would turn round in the middle of a street.
TEST(NextGta5Link, PrefersAnyRoadOverTheOneItCameFrom) {
    const Gta5Roads roads = RightTurn();
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(NextGta5Link(roads, 1u, 0u), 2u);
    }
}

// At a dead end there is nothing else, so it turns back.
TEST(NextGta5Link, TurnsBackAtADeadEnd) {
    const Gta5Roads roads = Straight();
    EXPECT_EQ(NextGta5Link(roads, 1u, 0u), 0u);
}

}  // namespace
}  // namespace sdl3cpp::services::impl
