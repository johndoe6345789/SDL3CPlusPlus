#pragma once

#include "services/interfaces/workflow/stunts/data/stunts_shape.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// `CAR<name>.RES` names the same car's body as `ST<name>.P3S`.
std::string StuntsCarBodyFileFor(const std::string& carResFile);

/**
 * @brief Loads a car's body from its own P3S archive.
 *
 * `car0` is the shape archive's highest-detail body entry; the ones
 * after it (`car1`, `car2`, ...) are lower-detail levels the game
 * itself would switch to at distance.
 */
StuntsShape LoadStuntsCarBody(const std::string& path);

}  // namespace sdl3cpp::services::impl
