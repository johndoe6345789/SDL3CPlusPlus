#include "services/interfaces/workflow/graphics/workflow_graphics_buffer_create_index_step.hpp"
#include "services/interfaces/workflow/graphics/graphics_index_buffer_upload.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowGraphicsBufferCreateIndexStep::WorkflowGraphicsBufferCreateIndexStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGraphicsBufferCreateIndexStep::GetPluginId() const {
    return "graphics.buffer.create_index";
}

void WorkflowGraphicsBufferCreateIndexStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string indicesKey =
        resolver.GetRequiredInputKey(step, "indices");
    const std::string outputHandleKey =
        resolver.GetRequiredOutputKey(step, "index_handle");

    const auto* indices_json = context.TryGet<nlohmann::json>(indicesKey);
    if (!indices_json || !indices_json->is_array()) {
        throw std::runtime_error(
            "graphics.buffer.create_index requires indices input "
            "(array of integers)");
    }

    const std::vector<uint16_t> index_data =
        ExtractIndexData(*indices_json);

    SDL_GPUDevice* device =
        context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error(
            "graphics.buffer.create_index: GPU device not found in "
            "context");
    }

    const IndexBufferUploadResult result =
        CreateAndUploadIndexBuffer(device, index_data);

    if (logger_) {
        logger_->Trace("WorkflowGraphicsBufferCreateIndexStep", "Execute",
                       "index_count=" + std::to_string(index_data.size()),
                       "Index buffer created successfully");
    }

    // Store buffer pointer in context
    context.Set<SDL_GPUBuffer*>("gpu_index_buffer", result.buffer);

    nlohmann::json buffer_data = {
        {"valid", true},
        {"index_count", index_data.size()},
        {"size_bytes", result.size_bytes}
    };
    context.Set(outputHandleKey, buffer_data);
}

}  // namespace sdl3cpp::services::impl
