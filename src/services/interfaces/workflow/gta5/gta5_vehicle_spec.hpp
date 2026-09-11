#pragma once

#include <glm/glm.hpp>

#include <string>

namespace sdl3cpp::services::impl {

/// Which vehicle to put on the road, every part read from the map.
struct Gta5VehicleSpec {
    /// The body's archetype, which is its .yft name: taxi_hi, the
    /// detailed model -- plain taxi is the low-detail one.
    std::string model;
    /// A wheel from vehiclemods/wheels_mods.rpf, such as wheel_spt_01. A
    /// vehicle's .yft carries no wheels; the game puts one of these at
    /// each wheel bone.
    std::string wheel;
    /// Body colour for vehicle_paint geometry. GTA V takes it from
    /// carcols.ymt, which is not in the extract.
    glm::vec3 paint{1.f};
    /// The real sizes are in vehicles.meta, not in the extract either;
    /// these suit a saloon.
    float wheelRadius{0.36f};
    float wheelWidth{0.25f};
};

}  // namespace sdl3cpp::services::impl
