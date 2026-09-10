#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Expands a leading `~` to `$HOME` (`$USERPROFILE` on Windows if `$HOME`
/// isn't set); a no-op if neither is set or `path` doesn't start with `~`.
std::string ResolveShaderPath(const std::string& path);

/// Reads the whole file at `path` (after `~`-expansion) into memory; throws
/// std::runtime_error (prefixed "graphics.gpu.shader.compile: ...") if it
/// can't be opened or read.
std::vector<uint8_t> LoadShaderBinary(const std::string& path);

}  // namespace sdl3cpp::services::impl
