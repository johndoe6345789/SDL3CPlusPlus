#pragma once

#include "services/interfaces/workflow/quake3/q3_md3_draw_params.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

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
                              const glm::mat4& view, const glm::vec3& camPos);

}  // namespace sdl3cpp::services::impl
