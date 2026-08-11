#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Generate cube vertex and index data on CPU.
 *
 * Produces 8-vertex position+color cube mesh and 36 triangle indices.
 * Stores results as JSON arrays in context for downstream GPU upload.
 *
 * Plugin ID: "geometry.cube.generate"
 *
 * Parameters:
 *   color_r (number, 0-255, default 255) - Red channel for vertex colors
 *   color_g (number, 0-255, default 255) - Green channel for vertex colors
 *   color_b (number, 0-255, default 255) - Blue channel for vertex colors
 *
 * Outputs:
 *   vertex_data   - JSON array of interleaved floats (x,y,z) + uint8 (r,g,b,a)
 *   index_data    - JSON array of uint16 triangle indices
 *   vertex_count  - int (8)
 *   index_count   - int (36)
 *   vertex_stride - int (sizeof PosColorVertex = 16 bytes)
 */
class WorkflowGeometryCubeGenerateStep final : public IWorkflowStep {
public:
    explicit WorkflowGeometryCubeGenerateStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
