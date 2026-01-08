#pragma once

#include "probe_types.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services {

struct SceneEntityIR {
    std::string id;
    std::vector<std::string> componentTypes;
    std::string jsonPath;
};

struct SceneIR {
    std::vector<SceneEntityIR> entities;
};

struct TextureIR {
    std::string id;
    std::string uri;
    std::string jsonPath;
};

struct MeshIR {
    std::string id;
    std::string uri;
    std::string jsonPath;
};

struct ShaderIR {
    std::string id;
    std::string vertexPath;
    std::string fragmentPath;
    std::string jsonPath;
};

struct MaterialIR {
    std::string id;
    std::string shader;
    std::unordered_map<std::string, std::string> textures;
    std::string jsonPath;
};

struct ResourceIR {
    std::vector<TextureIR> textures;
    std::vector<MeshIR> meshes;
    std::vector<ShaderIR> shaders;
    std::vector<MaterialIR> materials;
};

enum class RenderResourceRefType {
    None,
    PassOutput,
    Swapchain
};

struct RenderResourceRef {
    RenderResourceRefType type = RenderResourceRefType::None;
    std::string passId;
    std::string outputName;
    std::string jsonPath;
};

struct RenderPassInputIR {
    std::string name;
    RenderResourceRef ref;
    std::string jsonPath;
};

struct RenderPassOutputIR {
    std::string name;
    bool isSwapchain = false;
    std::string format;
    std::string usage;
    std::string jsonPath;
};

struct RenderPassIR {
    std::string id;
    std::string type;
    std::vector<RenderPassInputIR> inputs;
    std::vector<RenderPassOutputIR> outputs;
    std::string jsonPath;
};

struct RenderGraphIR {
    std::vector<RenderPassIR> passes;
};

struct RenderGraphBuildResult {
    std::vector<std::string> passOrder;
    std::vector<ProbeReport> diagnostics;
    bool success = true;
};

struct ConfigCompilerResult {
    SceneIR scene;
    ResourceIR resources;
    RenderGraphIR renderGraph;
    RenderGraphBuildResult renderGraphBuild;
    std::vector<ProbeReport> diagnostics;
    bool success = true;
};

}  // namespace sdl3cpp::services
