#pragma once

namespace sdl3cpp::q3 {

/// bsp.load scales Quake geometry by 1/32, so one engine unit is one
/// metre and 32 Quake units.
inline constexpr float kQuakeUnitsPerEngineUnit = 32.0f;

/// Convert a Quake speed, distance or acceleration into engine units.
constexpr float FromQuakeUnits(float quakeUnits) {
    return quakeUnits / kQuakeUnitsPerEngineUnit;
}

// Quantities carrying a length scale with the map...
inline constexpr float kMaxSpeed = FromQuakeUnits(320.0f);      // g_speed
inline constexpr float kStopSpeed = FromQuakeUnits(100.0f);     // pm_stopspeed
inline constexpr float kJumpVelocity = FromQuakeUnits(270.0f);  // JUMP_VELOCITY
inline constexpr float kGravity = FromQuakeUnits(800.0f);       // g_gravity

// ...while these are per-second coefficients with no length in them, so
// they are used exactly as ioq3 states them. Scaling these by the map
// factor is what made movement feel wrong: acceleration five times too
// sharp and friction a third too strong.
inline constexpr float kFriction = 6.0f;         // pm_friction
inline constexpr float kAccelerate = 10.0f;      // pm_accelerate
inline constexpr float kAirAccelerate = 1.0f;    // pm_airaccelerate
inline constexpr float kDuckScale = 0.25f;       // pm_duckScale

}  // namespace sdl3cpp::q3
