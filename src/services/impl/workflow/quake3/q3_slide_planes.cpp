#include "services/interfaces/workflow/quake3/q3_slide_planes.hpp"

namespace sdl3cpp::q3 {
namespace {

bool EntersPlane(const glm::vec3& velocity, const glm::vec3& plane) {
    return glm::dot(velocity, plane) < kIntoEpsilon;
}

}  // namespace

glm::vec3 ClipVelocity(const glm::vec3& velocity, const glm::vec3& normal,
                       float overbounce) {
    const float backoff = glm::dot(velocity, normal) * overbounce;
    return velocity - normal * backoff;
}

glm::vec3 ResolveAgainstPlanes(const glm::vec3& velocity,
                               const glm::vec3* planes, int numPlanes) {
    for (int i = 0; i < numPlanes; ++i) {
        if (!EntersPlane(velocity, planes[i])) {
            continue;
        }

        glm::vec3 clipped = ClipVelocity(velocity, planes[i], kOverclip);

        for (int j = 0; j < numPlanes; ++j) {
            if (j == i || !EntersPlane(clipped, planes[j])) {
                continue;
            }

            clipped = ClipVelocity(clipped, planes[j], kOverclip);
            if (glm::dot(clipped, planes[i]) >= 0.0f) {
                continue;  // no longer re-entering the first plane
            }

            const glm::vec3 crease =
                glm::normalize(glm::cross(planes[i], planes[j]));
            clipped = crease * glm::dot(crease, velocity);

            for (int k = 0; k < numPlanes; ++k) {
                if (k == i || k == j || !EntersPlane(clipped, planes[k])) {
                    continue;
                }
                return glm::vec3(0.0f);  // stop dead in a real corner
            }
        }

        return clipped;
    }
    return velocity;
}

}  // namespace sdl3cpp::q3
