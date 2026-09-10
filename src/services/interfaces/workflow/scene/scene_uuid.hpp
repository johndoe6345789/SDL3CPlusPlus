#pragma once

#include <string>

namespace sdl3cpp::services::impl {

/// Generates a new random UUID string (Windows RPC UuidCreate, or
/// libuuid's uuid_generate elsewhere), for scene object ids.
std::string GenerateSceneObjectUuid();

}  // namespace sdl3cpp::services::impl
