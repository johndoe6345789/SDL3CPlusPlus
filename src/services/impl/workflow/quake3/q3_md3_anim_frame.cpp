#include "services/interfaces/workflow/quake3/q3_md3_anim_frame.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

int ResolveMd3AnimFrame(const WorkflowContext& context,
                        const Md3DrawParams& params, int numFrames) {
    int frame = 0;
    if (!params.frameKey.empty()) {
        frame = context.Get<int>(params.frameKey, 0);
    } else {
        const double elapsed = context.GetDouble("frame.elapsed", 0.0);
        const int totalFrame = static_cast<int>(elapsed * params.fps);

        if (params.animCount > 0) {
            frame = params.animFirst + (totalFrame % params.animCount);
        } else {
            frame = totalFrame % numFrames;
        }
    }
    return std::max(0, std::min(frame, numFrames - 1));
}

}  // namespace sdl3cpp::services::impl
