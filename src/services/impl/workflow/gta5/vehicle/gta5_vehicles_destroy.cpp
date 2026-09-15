#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_seat.hpp"

#include "services/interfaces/workflow/gta5/effects/gta5_effects_spawn.hpp"

namespace sdl3cpp::services::impl {

void DestroyGta5Vehicles(Gta5StreamState& state,
                         btDiscreteDynamicsWorld* world,
                         Gta5Effects* effects) {
    // The traffic first: its cars are the ones the world is still
    // driving, and a mark riding one has to be let go of before the
    // body under it is deleted.
    for (Gta5TrafficCar& driven : state.traffic.cars) {
        if (effects) DropGta5MarksOn(*effects, driven.car.chassis);
        DestroyGta5Vehicle(driven.car, world);
    }
    state.traffic.cars.clear();
    state.traffic.junctions.clear();
    for (Gta5Vehicle& car : state.vehicles) {
        if (effects) DropGta5MarksOn(*effects, car.chassis);
        DestroyGta5Vehicle(car, world);
    }
    state.vehicles.clear();
    state.seated = -1;
}

}  // namespace sdl3cpp::services::impl
