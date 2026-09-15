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

// The player's box, from ioq3 g_client.c playerMins/playerMaxs. The
// origin sits 24 units above the feet, not at them: every spawn point,
// step and ceiling in a Quake map is authored around that, so getting
// it wrong misplaces the player against all of the geometry at once.
inline constexpr float kPlayerHalfWidth = FromQuakeUnits(15.0f);
inline constexpr float kPlayerFeet = FromQuakeUnits(-24.0f);
inline constexpr float kPlayerHead = FromQuakeUnits(32.0f);
inline constexpr float kPlayerCrouchHead = FromQuakeUnits(16.0f);

// bg_public.h DEFAULT_VIEWHEIGHT / CROUCH_VIEWHEIGHT, above the origin.
inline constexpr float kViewHeight = FromQuakeUnits(26.0f);
inline constexpr float kCrouchViewHeight = FromQuakeUnits(12.0f);

}  // namespace sdl3cpp::q3
