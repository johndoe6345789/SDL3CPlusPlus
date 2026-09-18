#pragma once

#include <string>

namespace sdl3cpp::fs2024 {

/// The float attribute `name="value"` inside `tag`, or `fallback` when
/// the attribute is absent. Shared by the small hand-rolled readers for
/// FS2024's vegetation XML files (attribute-heavy, not worth a real XML
/// parser for).
float Attr(const std::string& tag, const std::string& name, float fallback);

/// The whole opening tag `<Name ...>` or self-closing `<Name .../>`
/// starting at `at`, so its attributes can be read with Attr().
std::string OpeningTag(const std::string& xml, std::size_t at);

}  // namespace sdl3cpp::fs2024
