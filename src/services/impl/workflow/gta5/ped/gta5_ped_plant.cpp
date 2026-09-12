#include "services/interfaces/workflow/gta5/ped/gta5_ped_gait.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

constexpr const char* kSoles[] = {"SKEL_L_Toe0", "SKEL_R_Toe0",
                                  "SKEL_L_Foot", "SKEL_R_Foot"};

/// The lowest of the four points a ped stands on, along its own up.
float Lowest(const Gta5Skeleton& s, const glm::vec3& up,
             const std::vector<glm::mat4>& pose) {
    float low = 0.f;
    bool any = false;
    for (const char* name : kSoles) {
        const int b = FindGta5Bone(s, name);
        if (b < 0 || static_cast<std::size_t>(b) >= pose.size()) continue;
        const float at = glm::dot(glm::vec3(pose[b][3]), up);
        low = any ? std::min(low, at) : at;
        any = true;
    }
    return low;
}

}  // namespace

float Gta5FootFloat(const Gta5Skeleton& s, const Gta5PedAxes& axes,
                    const std::vector<glm::mat4>& pose) {
    // What the ped is drawn standing on is its bind pose's lowest point,
    // so any daylight between that and the posed foot is daylight under
    // the shoe. A bent knee through the stance shortens the leg by a few
    // centimetres, and a run lifts him a good deal further than that.
    return Lowest(s, axes.up, pose) - Lowest(s, axes.up, s.rest);
}

}  // namespace sdl3cpp::services::impl
