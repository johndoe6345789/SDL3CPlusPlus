#pragma once

#include <cstdint>
#include <unordered_set>

namespace sdl3cpp::services::impl {

/// The vehicle_paint shaders (and vehicle_mesh), by name and file name:
/// their colour is the vehicle's, not the texture's.
const std::unordered_set<std::uint32_t>& Gta5PaintShaderHashes();

/// GTA's emissive shaders, by name and by file name.
const std::unordered_set<std::uint32_t>& Gta5EmissiveShaderHashes();

}  // namespace sdl3cpp::services::impl
