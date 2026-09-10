#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// Stringifies a std::any holding std::string/double/int/bool; returns ""
/// for any other held type.
std::string AnyToString(const std::any& value);

/**
 * @brief Expands every `{name}` placeholder in `templateContent`.
 *
 * Each placeholder is resolved by looking up `name` directly in context
 * (via TryGetAny + AnyToString); if not found there and `valuesKey` names
 * an `unordered_map<string, string>` context entry, that map is checked
 * next.
 *
 * @param valuesKey Context key of an optional values map, or empty to
 *                   skip that fallback.
 * @throws std::runtime_error if a placeholder resolves in neither place.
 */
std::string InterpolateTemplate(WorkflowContext& context,
                                const std::string& templateContent,
                                const std::string& valuesKey);

}  // namespace sdl3cpp::services::impl
