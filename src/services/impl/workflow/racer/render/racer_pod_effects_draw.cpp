#include "services/interfaces/workflow/racer/render/racer_model_draw.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {


int DrawFlames(RacerDrawPass& d, const RacerWorldState& state,
               const RacerPodState& pod, const glm::mat4& at,
               const RacerPodRig& rig) {
    // Exhausts burn longer with speed, longer still and blue-white
    // under boost, and flicker a little frame to frame.
    const float pace =
        std::clamp(pod.speed / state.podSpec.topSpeed, 0.f, 1.4f);
    const float flicker = 0.9f + 0.1f * std::sin(state.race.raceTime * 61.f);
    const float length =
        (0.8f + 4.5f * pace) * (pod.boosting ? 1.7f : 1.f) * flicker;
    const RacerGpuModel& flame =
        pod.boosting ? state.effects.boostFlame : state.effects.flame;
    int drawn = 0;
    for (const glm::vec3& exhaust : rig.exhausts) {
        glm::mat4 m = glm::translate(at, exhaust);
        // The cone points along +z; the pod's tail is -z in its model.
        m = glm::scale(m, glm::vec3(rig.exhaustRadius, rig.exhaustRadius,
                                    -length));
        drawn += DrawRacerModel(d, flame, m, true);
    }
    return drawn;
}

int DrawShadow(RacerDrawPass& d, const RacerWorldState& state,
               const RacerPodState& pod, const RacerPodRig& rig) {
    const auto ground = RacerGroundHeight(state.ground, pod.position.x,
                                          pod.position.z, pod.position.y);
    if (!ground || pod.position.y - *ground > 25.f) return 0;
    glm::mat4 m = glm::translate(
        glm::mat4(1.f), glm::vec3(pod.position.x, *ground + 0.08f,
                                  pod.position.z));
    // The pod's own footprint, centred between engine tips and cockpit.
    m = glm::rotate(m, -pod.heading, glm::vec3(0.f, 1.f, 0.f));
    m = glm::translate(m, glm::vec3(0.f, 0.f, 0.5f * (rig.back - rig.front)));
    m = glm::scale(m, glm::vec3(2.f * rig.halfWidth, 1.f, rig.length));
    return DrawRacerModel(d, state.effects.shadow, m, true);
}

}  // namespace

int DrawRacerPodEffects(RacerDrawPass& d, const RacerWorldState& state,
                        const RacerPodState& pod, float roll,
                        const RacerPodRig& rig, bool blended) {
    const glm::mat4 at = RacerPodMatrix(pod, roll);
    if (!blended) return DrawRacerModel(d, rig.cables, at, false);
    int drawn = DrawShadow(d, state, pod, rig);
    drawn += DrawRacerModel(d, rig.binder, at, true);
    return drawn + DrawFlames(d, state, pod, at, rig);
}

}  // namespace sdl3cpp::services::impl
