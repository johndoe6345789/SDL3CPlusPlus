#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_traffic.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.traffic
 *
 * Cars driving themselves around the player, on GTA's own road network
 * (roads_dir, the same nodes*.ynd the shop parks against), obeying the
 * lights at every crossing the network describes -- any node with three
 * or more ways out of it.
 *
 * They keep to the right-hand lane, hold back off the car in front, and
 * stop at the line for a red. The lights themselves are drawn as a lamp
 * over each crossing, red, amber or green, so the cycle is visible from
 * the pavement as well as from behind a wheel.
 *
 * Parameters: model (the car to drive, a .yft archetype), cars (how many
 * to keep around the player), cycle and amber (seconds), and near/far
 * (metres: the ring they appear in and the range they are kept to).
 */
class WorkflowGta5TrafficStep final : public IWorkflowStep {
public:
    WorkflowGta5TrafficStep(std::shared_ptr<ILogger> logger,
                            std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    bool Load(const WorkflowStepDefinition& step, WorkflowContext& context);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    Gta5Roads roads_;
    Gta5Traffic traffic_;
    bool tried_{false};
};

}  // namespace sdl3cpp::services::impl
