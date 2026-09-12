#include "services/interfaces/workflow/gta5/ped/gta5_skeleton.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace sdl3cpp::services::impl {
namespace {

constexpr std::uint16_t kMaxBones = 1024;  // past this it is a bad read

glm::vec3 Vec3(const Gta5Resource& res, std::int64_t at) {
    return {res.F32(at), res.F32(at + 4), res.F32(at + 8)};
}

/// Translation, then rotation (a quaternion x, y, z, w), then scale.
glm::mat4 Local(const Gta5Resource& res, std::int64_t bone) {
    const glm::vec3 q = Vec3(res, bone);
    const glm::quat turn(res.F32(bone + 0x0C), q.x, q.y, q.z);
    return glm::translate(glm::mat4(1.f), Vec3(res, bone + 0x10)) *
           glm::mat4_cast(glm::normalize(turn)) *
           glm::scale(glm::mat4(1.f), Vec3(res, bone + 0x20));
}

}  // namespace

bool ReadGta5Skeleton(const Gta5Resource& res, std::int64_t drawable,
                      Gta5Skeleton& out) {
    out = Gta5Skeleton{};
    const std::int64_t skeleton = res.Follow(drawable + 0x18);
    const std::int64_t bones = skeleton < 0 ? -1 : res.Follow(skeleton + 0x20);
    if (bones < 0) return false;
    const std::uint16_t count = res.U16(skeleton + 0x5E);
    if (count == 0 || count > kMaxBones) return false;
    for (std::uint16_t i = 0; i < count; ++i) {
        const std::int64_t bone = bones + 0x50 * std::int64_t{i};
        const std::int64_t name = res.Follow(bone + 0x38);
        out.names.push_back(name < 0 ? std::string() : res.String(name));
        const auto parent = static_cast<std::int16_t>(res.U16(bone + 0x32));
        out.parents.push_back(parent >= 0 && parent < i ? parent : -1);
        out.locals.push_back(Local(res, bone));
    }
    ComposeGta5Pose(out, out.locals, out.rest);
    for (const glm::mat4& pose : out.rest) {
        out.unbind.push_back(glm::inverse(pose));
    }
    return true;
}

int FindGta5Bone(const Gta5Skeleton& skeleton, const std::string& name) {
    for (std::size_t i = 0; i < skeleton.names.size(); ++i) {
        if (skeleton.names[i] == name) return static_cast<int>(i);
    }
    return -1;
}

void ComposeGta5Pose(const Gta5Skeleton& skeleton,
                     const std::vector<glm::mat4>& locals,
                     std::vector<glm::mat4>& out) {
    out.resize(locals.size());
    for (std::size_t i = 0; i < locals.size(); ++i) {
        const int parent = skeleton.parents[i];
        out[i] = parent >= 0 ? out[parent] * locals[i] : locals[i];
    }
}

}  // namespace sdl3cpp::services::impl
