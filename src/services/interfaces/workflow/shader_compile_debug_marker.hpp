#pragma once

#include <string>

namespace sdl3cpp::services::impl {

/// Writes `content` to `path`, swallowing any exception (a best-effort
/// debug marker; shader.compile has always tolerated a read-only or
/// missing `test_outputs/` directory).
void WriteShaderCompileDebugMarker(const std::string& path,
                                   const std::string& content);

}  // namespace sdl3cpp::services::impl
