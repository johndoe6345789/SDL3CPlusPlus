#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_seat.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {

int StealGta5TrafficCar(Gta5StreamState& state, const btVector3& near,
                        float reach) {
    // The nearest one the game is driving, if it is within arm's reach.
    std::size_t best = state.traffic.cars.size();
    float closest = reach;
    for (std::size_t i = 0; i < state.traffic.cars.size(); ++i) {
        const Gta5Vehicle& car = state.traffic.cars[i].car;
        if (!car.chassis) continue;
        const float away =
            car.chassis->getWorldTransform().getOrigin().distance(near);
        if (away >= closest) continue;
        closest = away;
        best = i;
    }
    if (best >= state.traffic.cars.size()) return -1;
    // Out of the traffic and into the player's own list: the AI stops
    // driving it the moment it leaves, because it drives that list.
    state.vehicles.push_back(std::move(state.traffic.cars[best].car));
    state.traffic.cars.erase(state.traffic.cars.begin() +
                             static_cast<std::ptrdiff_t>(best));
    return static_cast<int>(state.vehicles.size()) - 1;
}

}  // namespace sdl3cpp::services::impl
