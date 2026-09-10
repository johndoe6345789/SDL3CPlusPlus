#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Builds the brush model pmove traces and hangs it off the world.
 *
 * Player movement traces the map's brush planes rather than the convex
 * hulls Bullet holds, because a plane trace reports the exact surface
 * that stopped a sweep and whether it began inside something, neither of
 * which a convex sweep can answer. The model is owned by the context
 * under "q3.brush_model"; the world only borrows it.
 *
 * @return how many brushes the model holds.
 */
size_t AttachPmoveBrushModel(btDiscreteDynamicsWorld* world,
                             const std::vector<uint8_t>& bspData, float scale,
                             WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
