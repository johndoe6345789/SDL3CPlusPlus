#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.md3.load
 *
 * Reads an MD3 model from the Q3 PK3 archive, decodes all animation frames,
 * and uploads per-frame vertex buffers + one index buffer per surface to the GPU.
 * Coordinates are converted from Q3 Z-up to the engine's Y-up system.
 *
 * Parameters:
 *   prefix  (string)  – context key namespace for this model, e.g. "lower"
 *   path    (string)  – PK3 entry path, e.g. "models/players/keel/lower.md3"
 *   skin    (string)  – optional explicit skin texture PK3 path
 *   anim    (string)  – optional animation.cfg PK3 path
 *
 * Context outputs (prefix = e.g. "lower"):
 *   q3.md3.{prefix}_num_frames       int
 *   q3.md3.{prefix}_num_surfs        int
 *   q3.md3.{prefix}_num_tags         int
 *   q3.md3.{prefix}_tags             nlohmann::json  (array[frame] → map tag_name → {origin,axis})
 *   q3.md3.{prefix}_anim             nlohmann::json  (parsed animation.cfg, optional)
 *   q3.md3.{prefix}_surf{s}_ib       SDL_GPUBuffer*
 *   q3.md3.{prefix}_surf{s}_num_idx  int
 *   q3.md3.{prefix}_surf{s}_f{f}_vb  SDL_GPUBuffer*
 *   q3.md3.{prefix}_surf{s}_tex      SDL_GPUTexture*
 *   q3.md3.{prefix}_surf{s}_samp     SDL_GPUSampler*
 *
 * Reads from context:
 *   q3.pk3_path   string   – path to pak0.pk3 (set by bsp.load)
 *   gpu_device    SDL_GPUDevice*
 */
class WorkflowQ3Md3LoadStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3Md3LoadStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
