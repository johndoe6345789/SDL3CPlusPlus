#pragma once

#include "services/interfaces/workflow/gta5/gta5_roads.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// A junction running a light. GTA's own network says where they are:
/// a car node with three or more ways out of it is a crossing.
struct Gta5Junction {
    std::uint32_t node{0};
    /// Where the map's own signal heads are, if this crossing has
    /// any. Nothing is drawn until one is found: GTA puts real lights
    /// out there, and a lamp hung in mid air over a node is not one.
    std::vector<glm::vec3> heads;
    float looked{-9.f};  // seconds since the map was last asked
    /// The bearing whose traffic goes first, radians, folded to half a
    /// turn -- a road and its opposite direction share one light.
    float axis{0.f};
    float clock{0.f};  // seconds into the cycle
};

/// A car the game drives, along the road graph in its own lane.
struct Gta5TrafficCar {
    std::uint32_t from{0};
    std::uint32_t to{0};
    float along{0.f};   // metres down the link it is on
    float speed{0.f};   // metres a second
    float want{9.f};    // what it would do with a clear road
    float lane{2.2f};   // metres right of the centre line
    Gta5Instance instance;
};

/// Everything the traffic needs between frames.
struct Gta5Traffic {
    std::vector<Gta5TrafficCar> cars;
    std::vector<Gta5Junction> junctions;
    Gta5Geometry* body{nullptr};
    float cycle{20.f};   // seconds a whole light cycle takes
    float amber{3.f};    // seconds of it that are the changeover
    int want{28};        // cars to keep around the player
    float near{40.f};    // no closer than this to spawn one
    float far{170.f};    // nor further than this, nor kept past it
    /// How far a car's own mesh reaches below its origin, so it can
    /// be set down on the road rather than buried to the sills in it.
    float lift{0.f};
};

/// Every crossing near `at` that is not already known, added.
void Gta5FindJunctions(Gta5Traffic& traffic, const Gta5Roads& roads,
                       const glm::vec3& at);

/// Junctions near `at` brought up to date, far ones dropped, and every
/// clock advanced. Call once a frame before the cars are driven.
void StepGta5Lights(Gta5Traffic& traffic, const Gta5Roads& roads,
                    const glm::vec3& at, float dt);

/// Whether a car arriving at `node` along `heading` may cross. Nodes
/// with no light -- anything that is not a junction -- are always open.
bool Gta5LightOpen(const Gta5Traffic& traffic, std::uint32_t node,
                   float heading);

/// Move every car a frame: down its link, into the next, stopping for
/// the car in front and for a light that is against it.
void DriveGta5Traffic(Gta5Traffic& traffic, const Gta5Roads& roads,
                      float dt);

/// Keep the road around `at` populated, and let go of what is behind.
void KeepGta5Traffic(Gta5Traffic& traffic, const Gta5Roads& roads,
                     const glm::vec3& at);

}  // namespace sdl3cpp::services::impl
