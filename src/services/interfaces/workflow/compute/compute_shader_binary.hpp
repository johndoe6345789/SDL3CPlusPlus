#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Expands a leading `~` against $HOME, as shell paths conventionally allow.
std::string ExpandComputeShaderPath(const std::string& path);

/**
 * @brief Reads a compiled compute shader binary from disk.
 * @throws std::runtime_error if the file cannot be opened.
 */
std::vector<uint8_t> LoadComputeShaderBinary(const std::string& path);

}  // namespace sdl3cpp::services::impl
