#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <glm/glm.hpp>

#include <string>

namespace sdl3cpp::services::impl {

/// q3.md3.draw's tunable parameters, each with the original defaults.
struct Md3DrawParams {
    std::string prefix   = "model";
    std::string posKey   = "";
    std::string yawKey    = "";
    std::string frameKey = "";
    float fps            = 15.0f;
    int animFirst         = 0;
    int animCount         = 0;
    bool viewmodel        = false;
    float vmRight        = 0.35f;
    float vmDown         = -0.3f;
    float vmFwd           = 0.5f;
};

Md3DrawParams ReadMd3DrawParams(const WorkflowStepDefinition& step);

/// Picks the frame to draw: `frameKey` if set, else an fps/animation-range
/// derived frame from `frame.elapsed`, clamped to [0, numFrames - 1].
int ResolveMd3AnimFrame(const WorkflowContext& context,
                        const Md3DrawParams& params, int numFrames);

/**
 * @brief Builds the world model matrix for a drawn MD3.
 *
 * In viewmodel mode the model is offset from the camera by the vm_*
 * parameters and oriented from the camera basis. Otherwise it is placed
 * at `posKey`'s position and rotated by `yawKey`'s yaw.
 *
 * Both branches remap to MD3/Quake's Z-up axes (X forward, Y left, Z up)
 * as ioq3's q_math.c AnglesToAxis() does: mapping Y to world up instead
 * rolls the model 90 degrees about its barrel and mirrors it, since
 * forward x up is -right there.
 */
glm::mat4 BuildMd3ModelMatrix(const WorkflowContext& context,
                              const Md3DrawParams& params,
                              const glm::mat4& view,
                              const glm::vec3& camPos);

}  // namespace sdl3cpp::services::impl
