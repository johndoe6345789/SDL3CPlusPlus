#include "services/interfaces/workflow/rendering/workflow_map_load_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <btBulletDynamicsCommon.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <nlohmann/json.hpp>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <vector>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowMapLoadStep::WorkflowMapLoadStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowMapLoadStep::GetPluginId() const {
    return "map.load";
}

void WorkflowMapLoadStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
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
    const float scale = getNum("scale", 1.0f);
    const bool create_physics = static_cast<int>(getNum("create_physics", 1)) != 0;

    if (file_path.empty()) {
        throw std::runtime_error("map.load: 'file_path' parameter is required");
    }

    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) throw std::runtime_error("map.load: GPU device not found");

    // Load scene with Assimp
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(file_path,
        aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices);

    if (!scene || !scene->mRootNode) {
        throw std::runtime_error("map.load: Failed to load '" + file_path + "': " +
                                 importer.GetErrorString());
    }

    // Vertex format: float3 pos + float2 uv = 20 bytes (matches textured pipeline)
    struct PosUvVertex { float x, y, z, u, v; };

    auto* world = context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);

    nlohmann::json mapNodes = nlohmann::json::array();
    int meshCount = 0;

    // Process each node in the scene
    std::function<void(const aiNode*, aiMatrix4x4)> processNode =
        [&](const aiNode* node, aiMatrix4x4 parentTransform) {

        aiMatrix4x4 transform = parentTransform * node->mTransformation;

        for (unsigned int m = 0; m < node->mNumMeshes; ++m) {
            const aiMesh* mesh = scene->mMeshes[node->mMeshes[m]];
            std::string meshName = node->mName.C_Str();
            if (meshName.empty()) meshName = "map_mesh_" + std::to_string(meshCount);

            // Extract vertices with transform applied
            std::vector<PosUvVertex> vertices;
            aiVector3D bbMin(1e9f, 1e9f, 1e9f), bbMax(-1e9f, -1e9f, -1e9f);

            for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
                aiVector3D pos = transform * mesh->mVertices[i];
                pos *= scale;

                PosUvVertex vert;
                vert.x = pos.x;
                vert.y = pos.y;
                vert.z = pos.z;
                if (mesh->mTextureCoords[0]) {
                    vert.u = mesh->mTextureCoords[0][i].x;
                    vert.v = mesh->mTextureCoords[0][i].y;
                } else {
                    vert.u = 0.0f;
                    vert.v = 0.0f;
                }
                vertices.push_back(vert);

                bbMin.x = std::min(bbMin.x, pos.x);
                bbMin.y = std::min(bbMin.y, pos.y);
                bbMin.z = std::min(bbMin.z, pos.z);
                bbMax.x = std::max(bbMax.x, pos.x);
                bbMax.y = std::max(bbMax.y, pos.y);
                bbMax.z = std::max(bbMax.z, pos.z);
            }

            std::vector<uint16_t> indices;
            for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
                const aiFace& face = mesh->mFaces[f];
                for (unsigned int j = 0; j < face.mNumIndices; ++j) {
                    indices.push_back(static_cast<uint16_t>(face.mIndices[j]));
                }
            }

            if (vertices.empty()) continue;

            // Upload to GPU
            uint32_t vtxSize = static_cast<uint32_t>(vertices.size() * sizeof(PosUvVertex));
            uint32_t idxSize = static_cast<uint32_t>(indices.size() * sizeof(uint16_t));

            SDL_GPUBufferCreateInfo vbInfo = {};
            vbInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
            vbInfo.size = vtxSize;
            SDL_GPUBuffer* vb = SDL_CreateGPUBuffer(device, &vbInfo);

            SDL_GPUBufferCreateInfo ibInfo = {};
            ibInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
            ibInfo.size = idxSize;
            SDL_GPUBuffer* ib = SDL_CreateGPUBuffer(device, &ibInfo);

            SDL_GPUTransferBufferCreateInfo tbInfo = {};
            tbInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            tbInfo.size = vtxSize + idxSize;
            SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device, &tbInfo);

            auto* mapped = static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(device, tb, false));
            std::memcpy(mapped, vertices.data(), vtxSize);
            std::memcpy(mapped + vtxSize, indices.data(), idxSize);
            SDL_UnmapGPUTransferBuffer(device, tb);

            SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
            SDL_GPUCopyPass* cp = SDL_BeginGPUCopyPass(cmd);

            SDL_GPUTransferBufferLocation srcV = {}; srcV.transfer_buffer = tb;
            SDL_GPUBufferRegion dstV = {}; dstV.buffer = vb; dstV.size = vtxSize;
            SDL_UploadToGPUBuffer(cp, &srcV, &dstV, false);

            SDL_GPUTransferBufferLocation srcI = {}; srcI.transfer_buffer = tb; srcI.offset = vtxSize;
            SDL_GPUBufferRegion dstI = {}; dstI.buffer = ib; dstI.size = idxSize;
            SDL_UploadToGPUBuffer(cp, &srcI, &dstI, false);

            SDL_EndGPUCopyPass(cp);
            SDL_SubmitGPUCommandBuffer(cmd);
            SDL_ReleaseGPUTransferBuffer(device, tb);

            // Store with plane_ prefix (compatible with draw.textured)
            context.Set<SDL_GPUBuffer*>("plane_" + meshName + "_vb", vb);
            context.Set<SDL_GPUBuffer*>("plane_" + meshName + "_ib", ib);
            context.Set("plane_" + meshName, nlohmann::json{
                {"vertex_count", vertices.size()},
                {"index_count", indices.size()},
                {"stride", 20}
            });

            // Create physics body from bounding box
            if (create_physics && world) {
                float cx = (bbMin.x + bbMax.x) * 0.5f;
                float cy = (bbMin.y + bbMax.y) * 0.5f;
                float cz = (bbMin.z + bbMax.z) * 0.5f;
                float hx = (bbMax.x - bbMin.x) * 0.5f;
                float hy = (bbMax.y - bbMin.y) * 0.5f;
                float hz = (bbMax.z - bbMin.z) * 0.5f;

                if (hx > 0.01f && hy > 0.01f && hz > 0.01f) {
                    auto* shape = new btBoxShape(btVector3(hx, hy, hz));
                    btTransform startTransform;
                    startTransform.setIdentity();
                    startTransform.setOrigin(btVector3(cx, cy, cz));

                    auto* motionState = new btDefaultMotionState(startTransform);
                    btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, motionState, shape);
                    auto* body = new btRigidBody(rbInfo);
                    world->addRigidBody(body);
                    context.Set<btRigidBody*>("physics_body_" + meshName, body);
                }
            }

            // Track for the frame loop draw step
            nlohmann::json nodeInfo = {
                {"name", meshName},
                {"index_count", indices.size()},
                {"bb_min", {bbMin.x, bbMin.y, bbMin.z}},
                {"bb_max", {bbMax.x, bbMax.y, bbMax.z}}
            };
            mapNodes.push_back(nodeInfo);
            meshCount++;
        }

        // Process children
        for (unsigned int c = 0; c < node->mNumChildren; ++c) {
            processNode(node->mChildren[c], transform);
        }
    };

    aiMatrix4x4 identity;
    processNode(scene->mRootNode, identity);

    context.Set("map.nodes", mapNodes);

    if (logger_) {
        logger_->Info("map.load: Loaded '" + file_path + "' (" +
                     std::to_string(meshCount) + " meshes, " +
                     std::to_string(scene->mNumMeshes) + " total in file)");
    }
}

}  // namespace sdl3cpp::services::impl
