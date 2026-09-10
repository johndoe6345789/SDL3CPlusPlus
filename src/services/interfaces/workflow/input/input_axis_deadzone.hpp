#pragma once

namespace sdl3cpp::services::impl {

/// Clamps `value` to [-1, 1], then rescales anything past `deadzone` back
/// onto the full [-1, 1] range (0 for anything within the deadzone).
float ApplyAxisDeadzone(float value, float deadzone);

}  // namespace sdl3cpp::services::impl
