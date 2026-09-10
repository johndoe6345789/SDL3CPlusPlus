#include "services/interfaces/workflow/rendering/map_load_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace sdl3cpp::services::impl {

MapLoadParams ReadMapLoadParams(const WorkflowStepDefinition& step,
                                const WorkflowContext& context) {
    WorkflowStepParameterResolver params;

    auto getStr = [&](const char* name, const std::string& def) {
        const auto* p = params.FindParameter(step, name);
        if (p && p->type == WorkflowParameterValue::Type::String) {
            return p->stringValue;
        }
        auto it = step.inputs.find(name);
        if (it != step.inputs.end()) {
            const auto* ctx = context.TryGet<std::string>(it->second);
            if (ctx) return *ctx;
        }
        return def;
    };
    auto getNum = [&](const char* name, float def) -> float {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number)
                   ? static_cast<float>(p->numberValue)
                   : def;
    };

    MapLoadParams result;
    result.filePath      = getStr("file_path", "");
    result.scale         = getNum("scale", 1.0f);
    result.createPhysics = static_cast<int>(getNum("create_physics", 1)) != 0;
    return result;
}

ExtractedMapMesh ExtractMapMeshGeometry(const aiMesh* mesh,
                                        const aiMatrix4x4& transform,
                                        float scale) {
    ExtractedMapMesh result;
    result.vertices.reserve(mesh->mNumVertices);

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
        result.vertices.push_back(vert);

        result.bbMin.x = std::min(result.bbMin.x, pos.x);
        result.bbMin.y = std::min(result.bbMin.y, pos.y);
        result.bbMin.z = std::min(result.bbMin.z, pos.z);
        result.bbMax.x = std::max(result.bbMax.x, pos.x);
        result.bbMax.y = std::max(result.bbMax.y, pos.y);
        result.bbMax.z = std::max(result.bbMax.z, pos.z);
    }

    for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
        const aiFace& face = mesh->mFaces[f];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            result.indices.push_back(static_cast<uint16_t>(face.mIndices[j]));
        }
    }
    return result;
}

MapMeshBuffers UploadMapMeshBuffers(SDL_GPUDevice* device,
                                    const std::vector<PosUvVertex>& vertices,
                                    const std::vector<uint16_t>& indices) {
    const uint32_t vtxSize =
        static_cast<uint32_t>(vertices.size() * sizeof(PosUvVertex));
    const uint32_t idxSize =
        static_cast<uint32_t>(indices.size() * sizeof(uint16_t));

    SDL_GPUBufferCreateInfo vbInfo = {};
    vbInfo.usage                   = SDL_GPU_BUFFERUSAGE_VERTEX;
    vbInfo.size                    = vtxSize;
    MapMeshBuffers result;
    result.vb = SDL_CreateGPUBuffer(device, &vbInfo);

    SDL_GPUBufferCreateInfo ibInfo = {};
    ibInfo.usage                   = SDL_GPU_BUFFERUSAGE_INDEX;
    ibInfo.size                    = idxSize;
    result.ib                      = SDL_CreateGPUBuffer(device, &ibInfo);

    SDL_GPUTransferBufferCreateInfo tbInfo = {};
    tbInfo.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbInfo.size                            = vtxSize + idxSize;
    SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device, &tbInfo);

    auto* mapped =
        static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(device, tb, false));
    std::memcpy(mapped, vertices.data(), vtxSize);
    std::memcpy(mapped + vtxSize, indices.data(), idxSize);
    SDL_UnmapGPUTransferBuffer(device, tb);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* cp       = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation srcV = {};
    srcV.transfer_buffer               = tb;
    SDL_GPUBufferRegion dstV           = {};
    dstV.buffer                        = result.vb;
    dstV.size                          = vtxSize;
    SDL_UploadToGPUBuffer(cp, &srcV, &dstV, false);

    SDL_GPUTransferBufferLocation srcI = {};
    srcI.transfer_buffer               = tb;
    srcI.offset                        = vtxSize;
    SDL_GPUBufferRegion dstI           = {};
    dstI.buffer                        = result.ib;
    dstI.size                          = idxSize;
    SDL_UploadToGPUBuffer(cp, &srcI, &dstI, false);

    SDL_EndGPUCopyPass(cp);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, tb);
    return result;
}

btRigidBody* CreateMapMeshPhysicsBody(btDiscreteDynamicsWorld* world,
                                      const glm::vec3& bbMin,
                                      const glm::vec3& bbMax) {
    if (!world) return nullptr;

    const float cx = (bbMin.x + bbMax.x) * 0.5f;
    const float cy = (bbMin.y + bbMax.y) * 0.5f;
    const float cz = (bbMin.z + bbMax.z) * 0.5f;
    const float hx = (bbMax.x - bbMin.x) * 0.5f;
    const float hy = (bbMax.y - bbMin.y) * 0.5f;
    const float hz = (bbMax.z - bbMin.z) * 0.5f;
    if (hx <= 0.01f || hy <= 0.01f || hz <= 0.01f) return nullptr;

    auto* shape = new btBoxShape(btVector3(hx, hy, hz));
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(cx, cy, cz));

    auto* motionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, motionState, shape);
    auto* body = new btRigidBody(rbInfo);
    world->addRigidBody(body);
    return body;
}

void ProcessMapSceneNode(const aiScene* scene, const aiNode* node,
                         aiMatrix4x4 parentTransform, SDL_GPUDevice* device,
                         btDiscreteDynamicsWorld* world,
                         const MapLoadParams& params, WorkflowContext& context,
                         nlohmann::json& mapNodes, int& meshCount) {
    const aiMatrix4x4 transform = parentTransform * node->mTransformation;

    for (unsigned int m = 0; m < node->mNumMeshes; ++m) {
        const aiMesh* mesh   = scene->mMeshes[node->mMeshes[m]];
        std::string meshName = node->mName.C_Str();
        if (meshName.empty()) {
            meshName = "map_mesh_" + std::to_string(meshCount);
        }

        ExtractedMapMesh extracted =
            ExtractMapMeshGeometry(mesh, transform, params.scale);
        if (extracted.vertices.empty()) continue;

        const MapMeshBuffers buffers =
            UploadMapMeshBuffers(device, extracted.vertices, extracted.indices);

        // Store with plane_ prefix (compatible with draw.textured).
        context.Set<SDL_GPUBuffer*>("plane_" + meshName + "_vb", buffers.vb);
        context.Set<SDL_GPUBuffer*>("plane_" + meshName + "_ib", buffers.ib);
        context.Set("plane_" + meshName,
                    nlohmann::json{{"vertex_count", extracted.vertices.size()},
                                   {"index_count", extracted.indices.size()},
                                   {"stride", 20}});

        if (params.createPhysics) {
            auto* body = CreateMapMeshPhysicsBody(world, extracted.bbMin,
                                                  extracted.bbMax);
            if (body) {
                context.Set<btRigidBody*>("physics_body_" + meshName, body);
            }
        }

        // Track for the frame loop draw step.
        nlohmann::json nodeInfo = {
            {"name", meshName},
            {"index_count", extracted.indices.size()},
            {"bb_min",
             {extracted.bbMin.x, extracted.bbMin.y, extracted.bbMin.z}},
            {"bb_max",
             {extracted.bbMax.x, extracted.bbMax.y, extracted.bbMax.z}}};
        mapNodes.push_back(nodeInfo);
        meshCount++;
    }

    for (unsigned int c = 0; c < node->mNumChildren; ++c) {
        ProcessMapSceneNode(scene, node->mChildren[c], transform, device, world,
                            params, context, mapNodes, meshCount);
    }
}

}  // namespace sdl3cpp::services::impl
