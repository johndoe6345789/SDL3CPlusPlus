#include "services/interfaces/workflow/rendering/workflow_model_load_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <nlohmann/json.hpp>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowModelLoadStep::WorkflowModelLoadStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowModelLoadStep::GetPluginId() const {
    return "model.load";
}

void WorkflowModelLoadStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepParameterResolver params;

    auto getStr = [&](const char* name, const std::string& def) -> std::string {
        const auto* p = params.FindParameter(step, name);
        if (p && p->type == WorkflowParameterValue::Type::String) return p->stringValue;
        auto it = step.inputs.find(name);
        if (it != step.inputs.end()) {
            const auto* ctx = context.TryGet<std::string>(it->second);
            if (ctx) return *ctx;
        }
        return def;
    };
    auto getNum = [&](const char* name, float def) -> float {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number) ? static_cast<float>(p->numberValue) : def;
    };

    const std::string file_path = getStr("file_path", "");
    const std::string name = getStr("name", "model");
    const float scale = getNum("scale", 1.0f);

    if (file_path.empty()) {
        throw std::runtime_error("model.load: 'file_path' parameter is required");
    }

    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error("model.load: GPU device not found in context");
    }

    // Load with Assimp
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(file_path,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices |
        aiProcess_PreTransformVertices);

    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
        throw std::runtime_error("model.load: Failed to load '" + file_path + "': " +
                                 importer.GetErrorString());
    }

    // Collect all mesh vertices/indices into a single buffer
    // Vertex format matches geometry.create_plane: float3 pos + float2 uv = 20 bytes
    struct PosUvVertex {
        float x, y, z;
        float u, v;
    };

    std::vector<PosUvVertex> vertices;
    std::vector<uint16_t> indices;
    uint16_t baseVertex = 0;

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];

        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            PosUvVertex vert;
            vert.x = mesh->mVertices[i].x * scale;
            vert.y = mesh->mVertices[i].y * scale;
            vert.z = mesh->mVertices[i].z * scale;
            if (mesh->mTextureCoords[0]) {
                vert.u = mesh->mTextureCoords[0][i].x;
                vert.v = mesh->mTextureCoords[0][i].y;
            } else {
                vert.u = 0.0f;
                vert.v = 0.0f;
            }
            vertices.push_back(vert);
        }

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            for (unsigned int j = 0; j < face.mNumIndices; ++j) {
                indices.push_back(static_cast<uint16_t>(baseVertex + face.mIndices[j]));
            }
        }
        baseVertex += static_cast<uint16_t>(mesh->mNumVertices);
    }

    if (vertices.empty()) {
        throw std::runtime_error("model.load: No vertices found in '" + file_path + "'");
    }

    const uint32_t vertex_count = static_cast<uint32_t>(vertices.size());
    const uint32_t index_count = static_cast<uint32_t>(indices.size());
    const uint32_t vertex_size = vertex_count * sizeof(PosUvVertex);
    const uint32_t index_size = index_count * sizeof(uint16_t);

    // Create GPU buffers (same pattern as geometry.create_plane)
    SDL_GPUBufferCreateInfo vbuf_info = {};
    vbuf_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vbuf_info.size = vertex_size;
    SDL_GPUBuffer* vertex_buffer = SDL_CreateGPUBuffer(device, &vbuf_info);

    SDL_GPUBufferCreateInfo ibuf_info = {};
    ibuf_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    ibuf_info.size = index_size;
    SDL_GPUBuffer* index_buffer = SDL_CreateGPUBuffer(device, &ibuf_info);

    if (!vertex_buffer || !index_buffer) {
        throw std::runtime_error("model.load: Failed to create GPU buffers");
    }

    // Upload via transfer buffer
    SDL_GPUTransferBufferCreateInfo tbuf_info = {};
    tbuf_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbuf_info.size = vertex_size + index_size;
    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &tbuf_info);

    auto* mapped = static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(device, transfer, false));
    std::memcpy(mapped, vertices.data(), vertex_size);
    std::memcpy(mapped + vertex_size, indices.data(), index_size);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation src_vert = {};
    src_vert.transfer_buffer = transfer;
    src_vert.offset = 0;
    SDL_GPUBufferRegion dst_vert = {};
    dst_vert.buffer = vertex_buffer;
    dst_vert.size = vertex_size;
    SDL_UploadToGPUBuffer(copy_pass, &src_vert, &dst_vert, false);

    SDL_GPUTransferBufferLocation src_idx = {};
    src_idx.transfer_buffer = transfer;
    src_idx.offset = vertex_size;
    SDL_GPUBufferRegion dst_idx = {};
    dst_idx.buffer = index_buffer;
    dst_idx.size = index_size;
    SDL_UploadToGPUBuffer(copy_pass, &src_idx, &dst_idx, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    // Store using same convention as geometry.create_plane (plane_ prefix)
    context.Set<SDL_GPUBuffer*>("plane_" + name + "_vb", vertex_buffer);
    context.Set<SDL_GPUBuffer*>("plane_" + name + "_ib", index_buffer);

    nlohmann::json meta = {
        {"vertex_count", vertex_count},
        {"index_count", index_count},
        {"stride", 20},
        {"meshes", scene->mNumMeshes},
        {"file", file_path}
    };
    context.Set("plane_" + name, meta);

    if (logger_) {
        logger_->Info("model.load: '" + name + "' loaded from " + file_path +
                     " (" + std::to_string(vertex_count) + " verts, " +
                     std::to_string(index_count) + " indices, " +
                     std::to_string(scene->mNumMeshes) + " meshes)");
    }
}

}  // namespace sdl3cpp::services::impl
