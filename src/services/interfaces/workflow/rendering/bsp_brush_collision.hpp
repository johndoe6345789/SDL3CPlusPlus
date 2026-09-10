#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <btBulletDynamicsCommon.h>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Two compound shapes built from a BSP's brush lump: one for brushes that
/// block everything, one (possibly null) for player-clip-only brushes.
struct BspBrushCollisionShapes {
    btCompoundShape* solid = nullptr;
    btCompoundShape* clip = nullptr;
    int solidBrushes = 0;
    int clipBrushes = 0;
    int skippedBrushes = 0;
};

/**
 * @brief Computes convex hull vertices from plane intersections of a brush.
 *
 * Intersects every triple of the brush's face planes and keeps the points
 * that lie on the inside of every other plane, i.e. the brush's own convex
 * hull expressed as points rather than half-spaces.
 */
std::vector<btVector3> ComputeBrushVertices(const BspBrushSide* sides,
                                            int numSides,
                                            const BspPlane* allPlanes,
                                            float scale);

/**
 * @brief Builds solid/player-clip compound shapes from a BSP's brush lump.
 *
 * Skips brushes whose shader has neither CONTENTS_SOLID nor
 * CONTENTS_PLAYERCLIP, is SURF_NODRAW (except player-clip, which is always
 * nodraw), or whose convex hull has fewer than 4 vertices. The returned
 * `clip` shape is null when no player-clip brush was found; the caller owns
 * both shapes (and, transitively, their child btConvexHullShapes).
 */
BspBrushCollisionShapes BuildBspBrushCollisionShapes(
    const std::vector<uint8_t>& bspData, float scale);

/// Removes `body` from `world` and deletes it, its motion state, and its
/// collision shape (recursing into a compound shape's child shapes).
void RemoveBspCollisionBody(btDiscreteDynamicsWorld* world,
                            btRigidBody*& body);

/// Wraps `shape` in a static btRigidBody with the given friction, adds it
/// to `world` with the default collision filters, and returns it.
btRigidBody* AddStaticCollisionBody(btDiscreteDynamicsWorld* world,
                                    btCollisionShape* shape, float friction);

/// Same as AddStaticCollisionBody, but adds the body with an explicit
/// collision `group`/`mask` pair (used for the player-clip body, which
/// only the character controller's queries should see).
btRigidBody* AddFilteredStaticCollisionBody(btDiscreteDynamicsWorld* world,
                                            btCollisionShape* shape,
                                            int group, int mask);

}  // namespace sdl3cpp::services::impl
