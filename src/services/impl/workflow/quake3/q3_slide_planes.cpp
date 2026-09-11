#include "services/interfaces/workflow/quake3/q3_slide_planes.hpp"

namespace sdl3cpp::q3 {
namespace {

bool EntersPlane(const glm::vec3& velocity, const glm::vec3& plane) {
    return glm::dot(velocity, plane) < kIntoEpsilon;
}

}  // namespace

glm::vec3 FaceNormal(const glm::vec3& normal) {
    if (normal.y >= kMinWalkNormal || normal.y <= -kMinWalkNormal) {
        return normal;  // ground or ceiling
    }
    glm::vec3 wall(normal.x, 0.0f, normal.z);
    const float length = glm::length(wall);
    return length > 1e-6f ? wall / length : normal;
}

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

            // Two planes facing each other -- the player squeezed against
            // a car's opposite sides -- have no crease to slide along, and
            // normalising their zero cross product made the player NaN.
            const glm::vec3 across = glm::cross(planes[i], planes[j]);
            const float length = glm::length(across);
            if (length < 1e-4f) return glm::vec3(0.0f);
            const glm::vec3 crease = across / length;
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
