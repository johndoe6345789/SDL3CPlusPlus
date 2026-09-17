#include "services/interfaces/workflow/fs2024/prepare/fs2024_gltf_scene.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>

namespace sdl3cpp::tools::fs2024 {
namespace {
using nlohmann::json;

glm::mat4 NodeLocalTransform(const json& node) {
    glm::vec3 t(0.f), s(1.f);
    glm::quat q(1.f, 0.f, 0.f, 0.f);
    if (node.contains("translation")) {
        const auto& a = node["translation"];
        t = {a[0].get<float>(), a[1].get<float>(), a[2].get<float>()};
    }
    if (node.contains("scale")) {
        const auto& a = node["scale"];
        s = {a[0].get<float>(), a[1].get<float>(), a[2].get<float>()};
    }
    if (node.contains("rotation")) {
        const auto& a = node["rotation"];  // glTF order: x, y, z, w
        q = glm::quat(a[3].get<float>(), a[0].get<float>(),
                     a[1].get<float>(), a[2].get<float>());
    }
    return glm::translate(glm::mat4(1.f), t) * glm::mat4_cast(q) *
          glm::scale(glm::mat4(1.f), s);
}

void Walk(const json& gltf, int nodeIndex, const glm::mat4& parent,
         std::vector<glm::mat4>& out) {
    const auto& node = gltf["nodes"][static_cast<std::size_t>(nodeIndex)];
    const glm::mat4 world = parent * NodeLocalTransform(node);
    out[static_cast<std::size_t>(nodeIndex)] = world;
    for (const auto& child : node.value("children", json::array())) {
        Walk(gltf, child.get<int>(), world, out);
    }
}

}  // namespace

std::vector<glm::mat4> ComputeNodeWorldTransforms(const json& gltf) {
    std::vector<glm::mat4> out(gltf.at("nodes").size(), glm::mat4(1.f));
    if (!gltf.contains("scenes")) return out;
    const auto& scene = gltf["scenes"][gltf.value("scene", 0)];
    for (const auto& root : scene.value("nodes", json::array())) {
        Walk(gltf, root.get<int>(), glm::mat4(1.f), out);
    }
    return out;
}

}  // namespace sdl3cpp::tools::fs2024
