#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.md3.upload_surfaces
 *
 * Uploads each MD3 surface to the GPU: one index buffer, one vertex buffer per
 * animation frame, and the surface's texture and sampler.  Vertices are decoded
 * from int16 and converted from Q3 Z-up to the engine's Y-up space.
 *
 * Writes to context, per surface s and frame f:
 *   q3.md3.{prefix}_surf{s}_ib       SDL_GPUBuffer*
 *   q3.md3.{prefix}_surf{s}_num_idx  int
 *   q3.md3.{prefix}_surf{s}_f{f}_vb  SDL_GPUBuffer*
 *   q3.md3.{prefix}_surf{s}_tex      SDL_GPUTexture*
 *   q3.md3.{prefix}_surf{s}_samp     SDL_GPUSampler*
 */
class WorkflowQ3Md3UploadSurfacesStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3Md3UploadSurfacesStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
