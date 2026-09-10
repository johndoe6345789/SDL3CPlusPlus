#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <cstring>
#include <stdexcept>

namespace sdl3cpp::services::impl {

GeometryPlaneParams ReadGeometryPlaneParams(
    const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;

    auto getNum = [&](const char* name, float def) -> float {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number)
                   ? static_cast<float>(p->numberValue)
                   : def;
    };
    auto getInt = [&](const char* pname, int def) -> int {
        const auto* p = params.FindParameter(step, pname);
        return (p && p->type == WorkflowParameterValue::Type::Number)
                   ? static_cast<int>(p->numberValue)
                   : def;
    };
    auto getStr = [&](const char* pname, const std::string& def) {
        const auto* p = params.FindParameter(step, pname);
        return (p && p->type == WorkflowParameterValue::Type::String)
                   ? p->stringValue
                   : def;
    };

    GeometryPlaneParams result;
    result.width      = getNum("width", 10.0f);
    result.depth      = getNum("depth", 10.0f);
    result.uvScaleX   = getNum("uv_scale_x", 1.0f);
    result.uvScaleY   = getNum("uv_scale_y", 1.0f);
    result.subdivisionsX = getInt("subdivisions_x", 1);
    result.subdivisionsY = getInt("subdivisions_y", 1);
    result.name       = getStr("name", "plane");
    return result;
}

GeometryPlaneMesh BuildGeometryPlaneMesh(const GeometryPlaneParams& params) {
    const float hw = params.width * 0.5f;
    const float hd = params.depth * 0.5f;
    const int vertsX = params.subdivisionsX + 1;
    const int vertsY = params.subdivisionsY + 1;

    GeometryPlaneMesh mesh;
    mesh.vertices.reserve(vertsX * vertsY);

    for (int iy = 0; iy < vertsY; ++iy) {
        float fy = static_cast<float>(iy) /
                   static_cast<float>(params.subdivisionsY);
        for (int ix = 0; ix < vertsX; ++ix) {
            float fx = static_cast<float>(ix) /
                       static_cast<float>(params.subdivisionsX);
            PlanePosUvVertex v;
            v.x = -hw + fx * params.width;
            v.y = 0.0f;
            v.z = -hd + fy * params.depth;
            v.u = fx * params.uvScaleX;
            v.v = fy * params.uvScaleY;
            mesh.vertices.push_back(v);
        }
    }

    mesh.indices.reserve(params.subdivisionsX * params.subdivisionsY * 6);
    for (int iy = 0; iy < params.subdivisionsY; ++iy) {
        for (int ix = 0; ix < params.subdivisionsX; ++ix) {
            uint16_t tl = static_cast<uint16_t>(iy * vertsX + ix);
            uint16_t tr = tl + 1;
            uint16_t bl = static_cast<uint16_t>((iy + 1) * vertsX + ix);
            uint16_t br = bl + 1;

            mesh.indices.push_back(tl);
            mesh.indices.push_back(bl);
            mesh.indices.push_back(tr);
            mesh.indices.push_back(tr);
            mesh.indices.push_back(bl);
            mesh.indices.push_back(br);
        }
    }
    return mesh;
}

GeometryPlaneBuffers UploadGeometryPlaneMesh(SDL_GPUDevice* device,
                                             const GeometryPlaneMesh& mesh) {
    const auto vertexCount = static_cast<uint32_t>(mesh.vertices.size());
    const auto indexCount  = static_cast<uint32_t>(mesh.indices.size());
    const uint32_t vertexSize = vertexCount * sizeof(PlanePosUvVertex);
    const uint32_t indexSize  = indexCount * sizeof(uint16_t);

    SDL_GPUBufferCreateInfo vbufInfo = {};
    vbufInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vbufInfo.size = vertexSize;
    SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(device, &vbufInfo);
    if (!vertexBuffer) {
        throw std::runtime_error(
            "geometry.create_plane: Failed to create vertex buffer");
    }

    SDL_GPUBufferCreateInfo ibufInfo = {};
    ibufInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    ibufInfo.size = indexSize;
    SDL_GPUBuffer* indexBuffer = SDL_CreateGPUBuffer(device, &ibufInfo);
    if (!indexBuffer) {
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        throw std::runtime_error(
            "geometry.create_plane: Failed to create index buffer");
    }

    SDL_GPUTransferBufferCreateInfo tbufInfo = {};
    tbufInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbufInfo.size = vertexSize + indexSize;
    SDL_GPUTransferBuffer* transfer =
        SDL_CreateGPUTransferBuffer(device, &tbufInfo);
    if (!transfer) {
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        SDL_ReleaseGPUBuffer(device, indexBuffer);
        throw std::runtime_error(
            "geometry.create_plane: Failed to create transfer buffer");
    }

    auto* mapped = static_cast<uint8_t*>(
        SDL_MapGPUTransferBuffer(device, transfer, false));
    std::memcpy(mapped, mesh.vertices.data(), vertexSize);
    std::memcpy(mapped + vertexSize, mesh.indices.data(), indexSize);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation srcVert = {};
    srcVert.transfer_buffer = transfer;
    srcVert.offset = 0;
    SDL_GPUBufferRegion dstVert = {};
    dstVert.buffer = vertexBuffer;
    dstVert.offset = 0;
    dstVert.size = vertexSize;
    SDL_UploadToGPUBuffer(copyPass, &srcVert, &dstVert, false);

    SDL_GPUTransferBufferLocation srcIdx = {};
    srcIdx.transfer_buffer = transfer;
    srcIdx.offset = vertexSize;
    SDL_GPUBufferRegion dstIdx = {};
    dstIdx.buffer = indexBuffer;
    dstIdx.offset = 0;
    dstIdx.size = indexSize;
    SDL_UploadToGPUBuffer(copyPass, &srcIdx, &dstIdx, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    GeometryPlaneBuffers result;
    result.vertexBuffer = vertexBuffer;
    result.indexBuffer  = indexBuffer;
    return result;
}

}  // namespace sdl3cpp::services::impl
