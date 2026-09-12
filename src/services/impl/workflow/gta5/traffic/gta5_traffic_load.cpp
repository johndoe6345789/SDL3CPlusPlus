#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_step.hpp"

#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"

namespace sdl3cpp::services::impl {

bool WorkflowGta5TrafficStep::Load(const WorkflowStepDefinition& step,
                                   WorkflowContext& context) {
    // The map has to be indexed before a car can be read out of it.
    if (!state_ || !state_->assets) return false;
    tried_ = true;
    traffic_.cycle = Gta5NumberOr(step, "cycle", traffic_.cycle);
    traffic_.amber = Gta5NumberOr(step, "amber", traffic_.amber);
    traffic_.near = Gta5NumberOr(step, "near", traffic_.near);
    traffic_.far = Gta5NumberOr(step, "far", traffic_.far);
    traffic_.mass = Gta5NumberOr(step, "mass", traffic_.mass);
    traffic_.want = Gta5ParameterOrInt(step, "cars", traffic_.want);
    traffic_.model = Gta5ParameterOr(step, "model", traffic_.model);
    traffic_.wheel = Gta5ParameterOr(step, "wheel", traffic_.wheel);
    traffic_.radius = Gta5NumberOr(step, "wheel_radius",
                                   traffic_.radius);
    traffic_.width = Gta5NumberOr(step, "wheel_width",
                                  traffic_.width);
    traffic_.ride = Gta5NumberOr(step, "ride_height",
                                 traffic_.ride);
    const std::string dir = Gta5ParameterOr(step, "roads_dir", "");
    if (!dir.empty()) LoadGta5Roads(dir, roads_);
    if (!roads_.loaded && logger_) {
        logger_->Warn("gta5.traffic: no road network in '" + dir +
                      "'; nothing to drive on");
    }
    if (logger_) {
        logger_->Info("gta5.traffic: " + std::to_string(roads_.nodes.size()) +
                      " road nodes, driving " + traffic_.model + " on " +
                      (traffic_.wheel.empty() ? "no wheels" : traffic_.wheel));
    }
    (void)context;
    return roads_.loaded;
}

}  // namespace sdl3cpp::services::impl
