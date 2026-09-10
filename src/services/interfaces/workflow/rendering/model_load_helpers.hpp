#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <assimp/scene.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// model.load's parsed parameters.
struct ModelLoadParams {
    std::string filePath;
    std::string name = "model";
    float scale       = 1.0f;
};

/// Reads `file_path`/`name`/`scale`, falling back from a step parameter
/// to the context key named by the matching `inputs` entry (file_path
/// and name only), then to the default.
ModelLoadParams ReadModelLoadParams(const WorkflowStepDefinition& step,
                                    const WorkflowContext& context);

/// Vertex format matching geometry.create_plane: float3 pos + float2 uv
/// = 20 bytes.
struct PosUvVertex {
    float x, y, z;
    float u, v;
};

/// A model.load mesh, flattened to one vertex/index buffer pair.
struct AssimpMeshData {
    std::vector<PosUvVertex> vertices;
    std::vector<uint16_t> indices;
};

/// Collects every mesh in `scene` into one vertex/index buffer, scaling
/// positions by `scale`. UVs default to (0, 0) for meshes with no UV
/// channel.
AssimpMeshData ExtractAssimpMeshData(const aiScene& scene, float scale);

/// Builds the "plane_<name>" metadata blob describing a loaded model.
nlohmann::json BuildModelLoadMetadata(uint32_t vertexCount,
                                      uint32_t indexCount,
                                      unsigned int meshCount,
                                      const std::string& filePath);

}  // namespace sdl3cpp::services::impl
