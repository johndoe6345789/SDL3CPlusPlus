#pragma once

#include "services/interfaces/workflow/gta5/gta5_resource.hpp"

#include <glm/glm.hpp>

#include <array>

namespace sdl3cpp::services::impl {

/// The four axle centres of a vehicle, from its fragment, in engine space
/// and turned to face +z like the mesh: front-right, front-left,
/// rear-right, rear-left -- the order AttachGta5Wheels wants, steered
/// pair first.
///
/// A vehicle's .yft has no wheels of its own; the game puts a wheel model
/// at each wheel bone. The fragment's physics LOD holds one transform per
/// child and one child per bone. FragType +0xF0 is the LOD group, +0x10
/// of that LOD 1; in the LOD, +0x30 is an offset added to every
/// transform, +0xD0 the children, +0x11D their count and +0x100 the
/// transform block, whose 64-byte matrices start at +0x20 with the
/// translation last. A child's bone tag is at +0x12. CodeWalker takes
/// wheel positions from here -- not from the mesh, whose brake discs sit
/// 0.14 m behind their own axle on the taxi.
bool ReadGta5FragmentAxles(const Gta5Resource& yft,
                           std::array<glm::vec3, 4>& out);

}  // namespace sdl3cpp::services::impl
