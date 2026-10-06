#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/racer/render/racer_panel.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// An image from the install (the title art, the hangar) as a panel,
/// with its own linear, mipmapped sampler.
bool CreateRacerImagePanel(SDL_GPUDevice* device,
                           const std::filesystem::path& path,
                           RacerPanel& out);

/// The full-screen rectangle that shows an image of `imageAspect`
/// filling a screen of `screenAspect`, cropping rather than stretching.
RacerScreenRect RacerCoverRect(float imageAspect, float screenAspect);

/**
 * Plugin ID: racer.screen.draw
 *
 * Draws whatever screen the flow is on over the frame: the title art
 * and menu, the hangar and pod shop, the loading screen, or the pause
 * and results panels over the race. Borrows the text overlay's pipeline
 * (run overlay.fps_init first) and runs after frame.gpu.end_scene.
 * Parameter: racer_dir (env RACER_DIR), for the install's art.
 */
class WorkflowRacerScreenStep final : public IWorkflowStep {
public:
    WorkflowRacerScreenStep(std::shared_ptr<ILogger> logger,
                            std::shared_ptr<RacerWorldState> state);
    ~WorkflowRacerScreenStep() override;
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    void Prepare(const WorkflowStepDefinition& step, SDL_GPUDevice* device,
                 SDL_GPUCommandBuffer* cmd, float aspect);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<RacerWorldState> state_;
    RacerPanel title_;
    RacerPanel hangar_;
    RacerPanel text_;
    std::string shown_;
    bool prepared_ = false;
};

}  // namespace sdl3cpp::services::impl
