#include "services/interfaces/workflow/rendering/model_load_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {

ModelLoadParams ReadModelLoadParams(const WorkflowStepDefinition& step,
                                    const WorkflowContext& context) {
    WorkflowStepParameterResolver params;
    auto getStr = [&](const char* name,
                      const std::string& def) -> std::string {
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

    ModelLoadParams out;
    out.filePath = getStr("file_path", "");
    out.name      = getStr("name", out.name);
    out.scale     = getNum("scale", out.scale);
    return out;
}

AssimpMeshData ExtractAssimpMeshData(const aiScene& scene, float scale) {
    AssimpMeshData out;
    uint16_t baseVertex = 0;

    for (unsigned int m = 0; m < scene.mNumMeshes; ++m) {
        const aiMesh* mesh = scene.mMeshes[m];

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
            out.vertices.push_back(vert);
        }

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            for (unsigned int j = 0; j < face.mNumIndices; ++j) {
                out.indices.push_back(
                    static_cast<uint16_t>(baseVertex + face.mIndices[j]));
            }
        }
        baseVertex += static_cast<uint16_t>(mesh->mNumVertices);
    }

    return out;
}

nlohmann::json BuildModelLoadMetadata(uint32_t vertexCount,
                                      uint32_t indexCount,
                                      unsigned int meshCount,
                                      const std::string& filePath) {
    return nlohmann::json{
        {"vertex_count", vertexCount},
        {"index_count", indexCount},
        {"stride", 20},
        {"meshes", meshCount},
        {"file", filePath},
    };
}

}  // namespace sdl3cpp::services::impl
