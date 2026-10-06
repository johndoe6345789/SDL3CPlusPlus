#pragma once

#include <cstdint>

namespace sdl3cpp::services::impl {

/// Vehicle-reaction flags a track surface carries (behaviour +0x2C),
/// as named by swe1r-assets. Only those the race uses are listed.
enum class RacerSurfaceFlag : std::uint32_t {
    ZeroGravityOn = 0x1,
    ZeroGravityOff = 0x2,
    Fast = 0x4,       ///< speed strips
    Slow = 0x8,       ///< sand and other drag
    Slip = 0x20,      ///< ice: little grip
    Wet = 0x100,
    Rough = 0x200,
    Swamp = 0x400,
    Lava = 0x2000,    ///< heats the engines
    Fall = 0x4000,    ///< a drop the pod cannot come back from
};

constexpr bool HasRacerSurfaceFlag(std::uint32_t flags,
                                   RacerSurfaceFlag flag) {
    return (flags & static_cast<std::uint32_t>(flag)) != 0;
}

}  // namespace sdl3cpp::services::impl
