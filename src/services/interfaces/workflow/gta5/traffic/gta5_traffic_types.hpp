#pragma once

#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_types.hpp"
#include "services/interfaces/workflow/gta5/world/gta5_roads.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// A junction running a light. GTA's own network says where they are:
/// a car node with three or more ways out of it, on real road.
struct Gta5Junction {
    std::uint32_t node{0};
    /// The bearing whose traffic goes first, radians, folded to half a
    /// turn -- a road and its opposite direction share one light.
    float axis{0.f};
    float clock{0.f};  // seconds into the cycle
    /// Where the map's own signal heads are, if this crossing has any.
    /// Nothing is drawn until one is found: GTA puts real lights out
    /// there, and a lamp hung in mid air over a node is not one.
    std::vector<glm::vec3> heads;
    float looked{-9.f};  // seconds since the map was last asked
};

/// A car the game drives: a raycast vehicle like the player's own, so
/// it rides on its suspension, rolls its wheels and collides with
/// whatever it meets. The AI only works the pedals and the wheel.
struct Gta5TrafficCar {
    Gta5Vehicle car;
    std::uint32_t from{0};   // the node it left
    std::uint32_t to{0};     // the node it is making for
    float want{9.f};         // what it would do with a clear road
    float follow{6.f};       // metres it stops behind what is ahead
    float nerve{1.f};        // above 1 takes bends faster
    float lane{2.2f};        // metres right of the centre line
    float stuck{0.f};        // seconds it has been going nowhere
};

/// One kind of car the traffic may put on the road.
struct Gta5TrafficModel {
    std::string model;
    std::string wheel;
    float radius{0.36f};
    float mass{1300.f};
};

/// Everything the traffic needs between frames.
struct Gta5Traffic {
    std::vector<Gta5TrafficCar> cars;
    std::vector<Gta5Junction> junctions;
    /// What it may drive. More than one, or every car on the road is
    /// the same car.
    std::vector<Gta5TrafficModel> models;
    float cycle{20.f};   // seconds a whole light cycle takes
    float amber{3.f};    // seconds of it that are the changeover
    int want{28};        // cars to keep around the player
    float near{40.f};    // no closer than this to spawn one
    float far{170.f};    // nor further than this, nor kept past it
    float ride{0.1f};    // how high a body sits on its wheels
    float width{0.25f};  // the wheel's width, metres
};

}  // namespace sdl3cpp::services::impl
