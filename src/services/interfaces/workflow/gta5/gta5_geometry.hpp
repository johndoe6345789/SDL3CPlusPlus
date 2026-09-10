#pragma once

#include <SDL3/SDL_gpu.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace sdl3cpp::services::impl {

/// Index buffers are 16-bit, so one archetype cannot exceed this many
/// vertices. Some GTA V building drawables do.
inline constexpr std::size_t kGta5MaxVerticesPerMesh = 65536u;

/// An archetype's mesh on the GPU, uploaded once and drawn many times.
///
/// references counts the live instances pointing at it. The buffers are
/// released when that reaches zero, so walking out of a district frees
/// its VRAM instead of holding every archetype ever seen.
struct Gta5Geometry {
    SDL_GPUBuffer* vertexBuffer{nullptr};
    SDL_GPUBuffer* indexBuffer{nullptr};
    std::uint32_t indexCount{0};
    int references{0};
    /// False once import or upload has failed, so a broken asset is not
    /// retried once per instance per frame.
    bool usable{false};
};

/// One placed copy of an archetype.
///
/// The geometry pointer addresses an element of the cache's
/// unordered_map, whose element addresses are stable across rehashing,
/// so it stays valid as more archetypes are cached.
struct Gta5Instance {
    Gta5Geometry* geometry{nullptr};
    std::array<float, 16> modelMatrix{};
};

}  // namespace sdl3cpp::services::impl
