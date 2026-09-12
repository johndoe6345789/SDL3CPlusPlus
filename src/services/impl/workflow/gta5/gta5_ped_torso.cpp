#include "services/interfaces/workflow/gta5/gta5_ped_gait.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {

void PoseGta5PedTorso(const Gta5Skeleton& s, const Gta5PedAxes& axes,
                      const Gta5Gait& gait, float phase, float amount,
                      float breath, std::vector<glm::mat4>& locals) {
    // He leans onto whichever foot is down. The rise and fall over the
    // step is not put in here: it falls out of the legs themselves once
    // the feet are planted, which is where a real one comes from.
    const float onto = gait.sway * amount * std::cos(phase);
    Gta5Shift(s, locals, "SKEL_ROOT",
              axes.right * onto + axes.up * (breath * 0.004f));
    // The hips turn with the leading leg and the chest turns against
    // them, which is the counter-rotation that makes a walk read as a
    // walk. The neck takes half of it back so he still faces his way.
    const float twist = gait.twist * amount * std::sin(phase);
    Gta5Turn(s, locals, "SKEL_Pelvis", Gta5About(-twist, axes.up));
    const float lean = gait.lean * amount + breath * 0.7f;
    Gta5Turn(s, locals, "SKEL_Spine_Root",
             Gta5About(twist * 0.8f, axes.up) *
                 Gta5About(-lean, axes.right));
    Gta5Turn(s, locals, "SKEL_Neck_1", Gta5About(-twist * 0.5f, axes.up));
}

}  // namespace sdl3cpp::services::impl
