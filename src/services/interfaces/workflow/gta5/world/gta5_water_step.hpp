#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.water.draw
 *
 * GTA V's sea, lakes and rivers: the quads of water_file (water.xml, from
 * common.rpf) as one mesh, uploaded once, drawn after the scene with
 * gpu_pipeline_gta5_water -- blended, depth-tested, not depth-written --
 * so the ground under it still shows through a little. Reads the
 * reflection pass's gta5.reflection.texture when there is one.
 */
class WorkflowGta5WaterDrawStep final : public IWorkflowStep {
public:
    WorkflowGta5WaterDrawStep(std::shared_ptr<ILogger> logger,
                              std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    void Load(const WorkflowStepDefinition& step, SDL_GPUDevice* device);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    SDL_GPUBuffer* vertices_{nullptr};
    std::uint32_t count_{0};
    bool tried_{false};
    bool staged_{false};  // uploaded with the next frame's batch
};

}  // namespace sdl3cpp::services::impl
