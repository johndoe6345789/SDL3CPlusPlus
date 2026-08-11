#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Take raw pixel data from context, encode to BMP via SDL, and write to disk.
 *
 * Plugin ID: image.encode.png
 *
 * Inputs:
 *   - pixel_data_key: context key holding std::vector<uint8_t> of raw ABGR8888 pixels
 *   - width:  image width  (context key holding uint32_t)
 *   - height: image height (context key holding uint32_t)
 *   - output_path: filesystem path to write the image to (string)
 *
 * Outputs:
 *   - success: bool indicating whether the file was written
 *
 * This step is a pure CPU operation, reusable for any image encoding task.
 * It resolves ~ in paths, creates parent directories, and uses SDL_SaveBMP
 * for zero-dependency image output. If the output_path ends with ".png",
 * the extension is replaced with ".bmp" (SDL only supports BMP natively).
 */
class WorkflowImageEncodePngStep final : public IWorkflowStep {
public:
    explicit WorkflowImageEncodePngStep(
        std::shared_ptr<ILogger> logger
    );

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
