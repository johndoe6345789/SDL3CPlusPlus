#include "services/interfaces/workflow/racer/player/racer_pod_report.hpp"

namespace sdl3cpp::services::impl {
namespace {

std::optional<float> Height(const void* ground, float x, float z,
                            float ceiling) {
    return RacerGroundHeight(*static_cast<const RacerGround*>(ground), x, z,
                             ceiling);
}

bool Wall(const void* ground, const glm::vec3& from, const glm::vec3& to) {
    return RacerWallBetween(*static_cast<const RacerGround*>(ground), from,
                            to);
}

}  // namespace

RacerSurface RacerGroundSurface(const RacerGround& ground) {
    return RacerSurface{&Height, &Wall, &ground};
}

}  // namespace sdl3cpp::services::impl
