#include "services/interfaces/workflow/gta5/gta5_garage.hpp"
#include "services/interfaces/workflow/gta5/gta5_settings.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_spec.hpp"

namespace sdl3cpp::services::impl {

bool ApplyGta5SavedCar(Gta5VehicleSpec& spec, float& mass,
                       const std::string& garageFile) {
    Gta5Settings settings;
    if (!LoadGta5Settings(settings) || settings.car.empty()) return false;
    const Gta5Garage garage = LoadGta5Garage(garageFile);
    for (const Gta5GarageCar& car : garage.cars) {
        if (car.model != settings.car) continue;
        spec.model = car.model + "_hi";
        spec.wheel = car.wheel;
        spec.wheelRadius = car.radius;
        mass = car.mass;
        if (settings.colour >= 0 &&
            settings.colour < static_cast<int>(garage.colours.size())) {
            spec.paint = garage.colours[settings.colour].rgb;
        }
        return true;
    }
    return false;
}

}  // namespace sdl3cpp::services::impl
