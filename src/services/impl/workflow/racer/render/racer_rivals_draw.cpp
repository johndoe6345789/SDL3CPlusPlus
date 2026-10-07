#include "services/interfaces/workflow/racer/render/racer_model_draw.hpp"

#include "services/interfaces/workflow/racer/player/racer_field_rules.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr float kHideMargin = 4.f;  // metres clear of a rival's body

/// A rival on top of the camera would fill the screen with the inside
/// of its engines; it is left out until clear of the eye.
bool CloseToEye(const RacerPodState& pod, const glm::vec3& eye) {
    for (const glm::vec3& c : RacerBodyCircles(pod)) {
        if (glm::distance(c, eye) < pod.bodyHalfWidth + kHideMargin) {
            return true;
        }
    }
    return false;
}

}  // namespace

int DrawRacerRivals(RacerDrawPass& d, const RacerWorldState& state,
                    const glm::vec3& eye, bool blended) {
    int drawn = 0;
    for (const RacerOpponent& opponent : state.opponents) {
        if (CloseToEye(opponent.pod, eye)) continue;
        drawn += DrawRacerModel(
            d, opponent.model, RacerPodMatrix(opponent.pod, opponent.roll),
            blended);
        drawn += DrawRacerPodEffects(d, state, opponent.pod,
                                     opponent.roll, opponent.rig,
                                     blended);
    }
    return drawn;
}

}  // namespace sdl3cpp::services::impl
