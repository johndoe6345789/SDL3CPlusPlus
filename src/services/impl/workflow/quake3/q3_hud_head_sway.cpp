#include "services/interfaces/workflow/quake3/q3_hud_head_sway.hpp"

#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

HeadAngles UpdateHeadSway(HeadSwayState& state, uint64_t nowMs) {
    if (nowMs >= state.swayEndMs) {
        // Pick new random target angles
        state.swayStartYaw   = state.swayEndYaw;
        state.swayStartPitch = state.swayEndPitch;
        state.swayStartMs    = state.swayEndMs;
        state.swayEndMs = nowMs + 100 + static_cast<uint64_t>(SDL_rand(2000));

        const float ryaw =
            (static_cast<float>(SDL_rand(1000)) / 1000.f - 0.5f) * 2.f;
        const float rpitch =
            (static_cast<float>(SDL_rand(1000)) / 1000.f - 0.5f) * 2.f;
        state.swayEndYaw   = glm::radians(180.f + 20.f * ryaw);
        state.swayEndPitch = glm::radians(5.f * rpitch);
    }
    if (state.swayStartMs == 0) state.swayStartMs = nowMs;

    float frac = 0.f;
    if (state.swayEndMs > state.swayStartMs) {
        frac = static_cast<float>(nowMs - state.swayStartMs) /
               static_cast<float>(state.swayEndMs - state.swayStartMs);
        frac = std::min(1.f, std::max(0.f, frac));
        frac = frac * frac * (3.f - 2.f * frac);  // smoothstep
    }

    HeadAngles out;
    out.yaw =
        state.swayStartYaw + (state.swayEndYaw - state.swayStartYaw) * frac;
    out.pitch = state.swayStartPitch +
                (state.swayEndPitch - state.swayStartPitch) * frac;
    return out;
}

glm::mat4 BuildHeadPortraitMvp(const HeadAngles& angles, float camDist) {
    const glm::vec3 camPos(
        std::sin(angles.yaw) * std::cos(angles.pitch) * camDist,
        -std::sin(angles.pitch) * camDist,
        std::cos(angles.yaw) * std::cos(angles.pitch) * camDist);

    const glm::mat4 view = glm::lookAt(camPos, glm::vec3(0.0f, 0.04f, 0.0f),
                                       glm::vec3(0.0f, 1.0f, 0.0f));
    // Q3A uses FOV=30° for head (tan(15°)=0.268); keep 30° here too.
    const glm::mat4 proj = glm::perspective(
        glm::radians(30.0f), 1.0f /*square aspect*/, 0.01f, 10.0f);
    const glm::mat4 model(1.0f);  // head at world origin
    return proj * view * model;
}

}  // namespace sdl3cpp::services::impl
