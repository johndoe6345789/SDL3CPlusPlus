#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_step.hpp"

#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_garage.hpp"

namespace sdl3cpp::services::impl {

bool WorkflowGta5TrafficStep::Load(const WorkflowStepDefinition& step,
                                   WorkflowContext& context) {
    // The map has to be indexed before a car can be read out of it.
    if (!state_ || !state_->assets) return false;
    tried_ = true;
    Gta5Traffic& traffic = state_->traffic;
    traffic.cycle = Gta5NumberOr(step, "cycle", traffic.cycle);
    traffic.amber = Gta5NumberOr(step, "amber", traffic.amber);
    traffic.near = Gta5NumberOr(step, "near", traffic.near);
    traffic.far = Gta5NumberOr(step, "far", traffic.far);
    traffic.ride = Gta5NumberOr(step, "ride_height", traffic.ride);
    traffic.width = Gta5NumberOr(step, "wheel_width", traffic.width);
    traffic.want = Gta5ParameterOrInt(step, "cars", traffic.want);
    // What it drives comes from the garage's own list, which is a set of
    // models already known to be in the extract and to have wheels that
    // fit them. Every car on the road being a taxi is not traffic.
    const std::string cars = Gta5ParameterOr(step, "cars_file", "");
    if (!cars.empty()) {
        const Gta5Garage garage = LoadGta5Garage(cars);
        for (const Gta5GarageCar& pick : garage.cars) {
            Gta5TrafficModel kind;
            kind.model = pick.model + "_hi";
            kind.wheel = pick.wheel;
            kind.radius = pick.radius;
            kind.mass = pick.mass > 0.f ? pick.mass : kind.mass;
            traffic.models.push_back(kind);
        }
    }
    if (traffic.models.empty()) {
        traffic.models.push_back(Gta5TrafficModel{"taxi_hi", "wheel_spt_01",
                                                  0.36f, 1300.f});
    }
    const std::string dir = Gta5ParameterOr(step, "roads_dir", "");
    if (!dir.empty()) LoadGta5Roads(dir, roads_);
    if (!roads_.loaded && logger_) {
        logger_->Warn("gta5.traffic: no road network in '" + dir +
                      "'; nothing to drive on");
    }
    if (logger_) {
        logger_->Info("gta5.traffic: " + std::to_string(roads_.nodes.size()) +
                      " road nodes, " +
                      std::to_string(traffic.models.size()) +
                      " kinds of car");
    }
    (void)context;
    return roads_.loaded;
}

}  // namespace sdl3cpp::services::impl
