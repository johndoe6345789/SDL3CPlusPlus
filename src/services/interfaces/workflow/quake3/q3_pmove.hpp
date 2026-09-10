#pragma once

#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"

namespace sdl3cpp::q3 {

/**
 * @brief One frame's movement intent, ioq3's usercmd_t in miniature.
 *
 * Quake's player and its bots do not have separate movement code: the
 * bot AI's only output is a usercmd, and the server runs the same
 * Pmove() on it that it runs on a human's keyboard (g_active.c
 * ClientThink_real, reached for bots via G_RunClient). Everything that
 * makes movement feel right — friction, acceleration, gravity, sliding
 * along planes, stepping up stairs — is then shared for free.
 *
 * forwardMove and rightMove are fractions in [-1, 1] rather than ioq3's
 * [-127, 127] bytes, since nothing here has to survive a network wire.
 */
struct Q3UserCmd {
    float forwardMove = 0.0f;
    float rightMove   = 0.0f;
    float yaw         = 0.0f;  // radians, the engine's yaw convention
    bool jump         = false;
};

/// bg_pmove.c PM_GroundTrace: records the plane underfoot, or applies
/// gravity for this frame when there is nothing under it.
void PmGroundTrace(services::impl::Q3PlayerState& ps,
                   btDiscreteDynamicsWorld* world, float dt,
                   const btCollisionObject* self);

/// bg_pmove.c PM_Friction, ground only.
void PmFriction(services::impl::Q3PlayerState& ps, float dt);

/// bg_pmove.c PM_Accelerate, with PM_WalkMove's projection of the
/// movement basis onto the ground plane.
void PmAccelerate(services::impl::Q3PlayerState& ps, const Q3UserCmd& cmd,
                  float dt);

/**
 * @brief Runs a whole frame of movement for one mover.
 *
 * The same sequence the q3.pm.* workflow steps run for the player, in
 * the same order, so anything driven by a Q3UserCmd moves identically
 * to the player. `self` is the mover's own collision object, which its
 * traces must ignore; null when it has none.
 */
void Q3PmoveOne(services::impl::Q3PlayerState& ps, const Q3UserCmd& cmd,
                btDiscreteDynamicsWorld* world, float dt,
                const btCollisionObject* self);

}  // namespace sdl3cpp::q3
