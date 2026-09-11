#include "services/interfaces/workflow/gta5/gta5_bone_pose.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace sdl3cpp::services::impl {
namespace {

constexpr std::uint16_t kMaxBones = 1024;  // past this it is a bad read

glm::vec3 Vec3(const Gta5Resource& res, std::int64_t at) {
    return {res.F32(at), res.F32(at + 4), res.F32(at + 8)};
}

}  // namespace

std::vector<glm::mat4> ReadGta5BonePose(const Gta5Resource& res,
                                        std::int64_t drawable) {
    std::vector<glm::mat4> pose;
    const std::int64_t skeleton = res.Follow(drawable + 0x18);
    const std::int64_t bones = skeleton < 0 ? -1 : res.Follow(skeleton + 0x20);
    if (bones < 0) return pose;
    const std::uint16_t count = res.U16(skeleton + 0x5E);
    if (count > kMaxBones) return pose;
    pose.reserve(count);
    for (std::uint16_t i = 0; i < count; ++i) {
        const std::int64_t bone = bones + 0x50 * std::int64_t{i};
        const glm::vec3 q = Vec3(res, bone);
        const glm::quat turn(res.F32(bone + 0x0C), q.x, q.y, q.z);
        const glm::mat4 local =
            glm::translate(glm::mat4(1.f), Vec3(res, bone + 0x10)) *
            glm::mat4_cast(glm::normalize(turn)) *
            glm::scale(glm::mat4(1.f), Vec3(res, bone + 0x20));
        const auto parent = static_cast<std::int16_t>(res.U16(bone + 0x32));
        pose.push_back(parent >= 0 && parent < i ? pose[parent] * local
                                                 : local);
    }
    return pose;
}

const glm::mat4* Gta5ModelPose(const Gta5Resource& res, std::int64_t model,
                               const std::vector<glm::mat4>& pose) {
    const std::uint32_t binding = res.U32(model + 0x28);
    const std::size_t bone = binding >> 24;
    if (((binding >> 8) & 0xFFu) != 0 || bone >= pose.size()) return nullptr;
    return &pose[bone];
}

void PoseGta5Part(Gta5SubMeshData& part, const glm::mat4& pose) {
    if (pose == glm::mat4(1.f)) return;
    const glm::mat3 turn(pose);
    // Engine space is GTA's (x, z, -y), so GTA's is the engine's
    // (x, -z, y): pose there and come back.
    for (BspRenderVertex& v : part.vertices) {
        const glm::vec4 p = pose * glm::vec4(v.x, -v.z, v.y, 1.f);
        v.x = p.x, v.y = p.z, v.z = -p.y;
        const glm::vec3 n = turn * glm::vec3(v.nx, -v.nz, v.ny);
        const float length = glm::length(n);
        if (length > 1e-6f) {
            v.nx = n.x / length, v.ny = n.z / length, v.nz = -n.y / length;
        }
    }
}

}  // namespace sdl3cpp::services::impl
