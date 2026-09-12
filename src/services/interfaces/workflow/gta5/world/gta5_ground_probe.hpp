#pragma once

#include <btBulletDynamicsCommon.h>

#include <string>

namespace sdl3cpp::services::impl {

/// What the physics world holds over a vertical column: how many objects
/// there are, how many have bounding boxes covering (x, z), how many of
/// those are scaled meshes, and the height range they span.
///
/// For when a downward ray finds nothing where the ground is drawn: it
/// tells "no collision here" apart from "collision the ray misses".
std::string DescribeGta5Column(btDiscreteDynamicsWorld* world, float x,
                               float z);

}  // namespace sdl3cpp::services::impl
