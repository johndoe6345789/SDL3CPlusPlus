#include "services/interfaces/workflow/geometry/workflow_geometry_create_cube_step.hpp"
#include "services/interfaces/workflow/geometry/cube_geometry_data.hpp"
#include "services/interfaces/workflow/geometry/geometry_create_cube_helpers.hpp"
#include "services/interfaces/workflow/graphics/graphics_buffer_upload_helpers.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowGeometryCreateCubeStep::WorkflowGeometryCreateCubeStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGeometryCreateCubeStep::GetPluginId() const {
    return "geometry.create_cube";
}

void WorkflowGeometryCreateCubeStep::Execute(const WorkflowStepDefinition&,
                                             WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowGeometryCreateCubeStep", "Execute", "",
                       "Entry");
    }

    try {
        auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
        if (!device) {
            throw std::runtime_error(
                "geometry.create_cube: GPU device not found in context");
        }

        const auto vertices = BuildRainbowCubeVertices();
        std::vector<uint8_t> vertexBytes(sizeof(vertices));
        std::memcpy(vertexBytes.data(), vertices.data(), sizeof(vertices));
        const std::vector<uint16_t> indexValues(kCubeIndices,
                                                kCubeIndices + 36);

        const UploadedGpuBuffers buffers =
            CreateAndUploadGpuBuffers(device, vertexBytes, indexValues);

        context.Set<SDL_GPUBuffer*>("gpu_vertex_buffer", buffers.vertexBuffer);
        context.Set<SDL_GPUBuffer*>("gpu_index_buffer", buffers.indexBuffer);

        context.Set("cube_mesh",
                    BuildUploadedMeshMetadata(
                        8, 36, static_cast<int>(sizeof(PosColorVertex))));
        context.Set("geometry_created", true);

        if (logger_) {
            logger_->Info(
                "WorkflowGeometryCreateCubeStep: Cube created (8 "
                "vertices, 36 indices, stride=" +
                std::to_string(sizeof(PosColorVertex)) + " bytes)");
        }
    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowGeometryCreateCubeStep::Execute: " +
                           std::string(e.what()));
        }
        context.Set("geometry_created", false);
    }
}

}  // namespace sdl3cpp::services::impl
