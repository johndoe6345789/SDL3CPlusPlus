#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_held_weapon.hpp"
#include "services/interfaces/workflow/gta5/gta5_ped.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.player.character
 *
 * The player's body in third person: a GTA ped (ped, from ped_dir, in
 * the components listed), skinned on the CPU each frame and walking by
 * code -- legs and arms swung in time with the ground covered -- facing
 * the way it moves. Drawn like the cars, through gta5.tiles.cull; hidden
 * in first person and in the car. Runs before the cull.
 */
class WorkflowGta5PlayerCharacterStep final : public IWorkflowStep {
public:
    WorkflowGta5PlayerCharacterStep(std::shared_ptr<ILogger> logger,
                                    std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    void Load(const WorkflowStepDefinition& step, SDL_GPUDevice* device);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    Gta5Ped ped_;
    Gta5Geometry weapon_;
    std::string weaponModel_;
    Gta5PedWalk walk_;
    std::vector<glm::mat4> skin_;
    std::vector<BspRenderVertex> skinned_;
    bool tried_{false};
    int frame_{0};
    float yaw_{3.14159265f};  // away from the camera at its first yaw
};

}  // namespace sdl3cpp::services::impl
