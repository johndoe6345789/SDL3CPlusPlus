#pragma once

#include <any>
#include <string>
#include <unordered_map>

namespace sdl3cpp::services::impl {

/// Stringifies a std::any holding string/bool/double/int, for
/// control.condition.switch's case matching (doubles are truncated to
/// long long, matching the original's integer-case convention).
/// @throws std::runtime_error for any other held type.
std::string SwitchValueToString(const std::any* value);

/**
 * @brief Picks the "case_<value>" input matching `valueStr`, else
 *        "default".
 *
 * `inputs` is a step's raw input map; entries other than "value",
 * "default", and "case_*" are ignored.
 *
 * @return The selected step id, or "" if neither a matching case nor a
 *         default input was present.
 */
std::string FindSwitchCaseStepId(
    const std::unordered_map<std::string, std::string>& inputs,
    const std::string& valueStr);

}  // namespace sdl3cpp::services::impl
