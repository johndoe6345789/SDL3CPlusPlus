#include "services/interfaces/workflow/geometry/workflow_geometry_create_cube_step.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_context.hpp"
#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <utility>
#include <cstring>

using json = nlohmann::json;

namespace sdl3cpp::services::impl {

WorkflowGeometryCreateCubeStep::WorkflowGeometryCreateCubeStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGeometryCreateCubeStep::GetPluginId() const {
    return "geometry.create_cube";
}

void WorkflowGeometryCreateCubeStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowGeometryCreateCubeStep", "Execute", "", "Entry");
    }

    // Cube vertex structure: position (xyz) + color (RGBA as 4 normalized bytes)
    // Layout matches pipeline: Float3 position at offset 0, UByte4Norm color at offset 12
    struct PosColorVertex {
        float x, y, z;
        uint8_t r, g, b, a;
    };

    // Cube vertices with per-vertex colors (8 corners)
    PosColorVertex s_cubeVertices[] = {
        {-1.0f,  1.0f,  1.0f,   0,   0,   0, 255 },  // Black
        { 1.0f,  1.0f,  1.0f, 255,   0,   0, 255 },  // Red
        {-1.0f, -1.0f,  1.0f,   0, 255,   0, 255 },  // Green
        { 1.0f, -1.0f,  1.0f, 255, 255,   0, 255 },  // Yellow
        {-1.0f,  1.0f, -1.0f,   0,   0, 255, 255 },  // Blue
        { 1.0f,  1.0f, -1.0f, 255,   0, 255, 255 },  // Magenta
        {-1.0f, -1.0f, -1.0f,   0, 255, 255, 255 },  // Cyan
        { 1.0f, -1.0f, -1.0f, 255, 255, 255, 255 },  // White
    };

    // Cube indices (12 triangles = 36 indices)
    const uint16_t s_cubeIndices[] = {
        0, 1, 2,  2, 1, 3,  // Front face
        4, 6, 5,  5, 6, 7,  // Back face
        0, 2, 4,  4, 2, 6,  // Left face
        1, 5, 3,  5, 7, 3,  // Right face
        0, 4, 1,  4, 5, 1,  // Top face
        2, 3, 6,  6, 3, 7,  // Bottom face
    };

    try {
        SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
        if (!device) {
            throw std::runtime_error("geometry.create_cube: GPU device not found in context");
        }

        uint32_t vertex_size = sizeof(s_cubeVertices);
        uint32_t index_size = sizeof(s_cubeIndices);

        // Create GPU vertex buffer
        SDL_GPUBufferCreateInfo vbuf_info = {};
        vbuf_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vbuf_info.size = vertex_size;

        SDL_GPUBuffer* vbuf = SDL_CreateGPUBuffer(device, &vbuf_info);
        if (!vbuf) {
            throw std::runtime_error("geometry.create_cube: Failed to create vertex buffer");
        }

        // Create GPU index buffer
        SDL_GPUBufferCreateInfo ibuf_info = {};
        ibuf_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        ibuf_info.size = index_size;

        SDL_GPUBuffer* ibuf = SDL_CreateGPUBuffer(device, &ibuf_info);
        if (!ibuf) {
            SDL_ReleaseGPUBuffer(device, vbuf);
            throw std::runtime_error("geometry.create_cube: Failed to create index buffer");
        }

        // Upload both buffers in one transfer
        uint32_t transfer_size = vertex_size + index_size;
        SDL_GPUTransferBufferCreateInfo transfer_info = {};
        transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transfer_info.size = transfer_size;

        SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
        if (!transfer) {
            SDL_ReleaseGPUBuffer(device, vbuf);
            SDL_ReleaseGPUBuffer(device, ibuf);
            throw std::runtime_error("geometry.create_cube: Failed to create transfer buffer");
        }

        void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
        memcpy(mapped, s_cubeVertices, vertex_size);
        memcpy(static_cast<uint8_t*>(mapped) + vertex_size, s_cubeIndices, index_size);
        SDL_UnmapGPUTransferBuffer(device, transfer);

        // Copy to GPU buffers
        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);

        // Upload vertex data
        SDL_GPUTransferBufferLocation v_src = {};
        v_src.transfer_buffer = transfer;
        v_src.offset = 0;

        SDL_GPUBufferRegion v_dst = {};
        v_dst.buffer = vbuf;
        v_dst.offset = 0;
        v_dst.size = vertex_size;

        SDL_UploadToGPUBuffer(copy_pass, &v_src, &v_dst, false);

        // Upload index data
        SDL_GPUTransferBufferLocation i_src = {};
        i_src.transfer_buffer = transfer;
        i_src.offset = vertex_size;

        SDL_GPUBufferRegion i_dst = {};
        i_dst.buffer = ibuf;
        i_dst.offset = 0;
        i_dst.size = index_size;

        SDL_UploadToGPUBuffer(copy_pass, &i_src, &i_dst, false);

        SDL_EndGPUCopyPass(copy_pass);
        SDL_SubmitGPUCommandBuffer(cmd);

        SDL_ReleaseGPUTransferBuffer(device, transfer);

        // Store buffer pointers in context for downstream steps
        context.Set<SDL_GPUBuffer*>("gpu_vertex_buffer", vbuf);
        context.Set<SDL_GPUBuffer*>("gpu_index_buffer", ibuf);

        // Also store metadata as JSON
        json geometry = {
            {"vertex_buffer_handle", {
                {"valid", true},
                {"vertex_count", 8}
            }},
            {"index_buffer_handle", {
                {"valid", true},
                {"index_count", 36}
            }},
            {"vertex_layout", {
                {"stride", static_cast<int>(sizeof(PosColorVertex))}
            }}
        };

        context.Set("cube_mesh", geometry);
        context.Set("geometry_created", true);

        if (logger_) {
            logger_->Info("WorkflowGeometryCreateCubeStep: Cube created (8 vertices, 36 indices, stride=" +
                         std::to_string(sizeof(PosColorVertex)) + " bytes)");
        }

    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowGeometryCreateCubeStep::Execute: " + std::string(e.what()));
        }
        context.Set("geometry_created", false);
    }
}

}  // namespace sdl3cpp::services::impl
