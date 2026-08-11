#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services {

/**
 * Shader system type definitions
 */

/// Shader type enumeration
enum class ShaderType {
    Vertex,
    Fragment,
    Compute,
    Unknown
};

/// Shader system identifier
using ShaderSystemId = std::string;

/// Shader program handle
struct ShaderProgramHandle {
    uint16_t handle = UINT16_MAX;

    bool IsValid() const { return handle != UINT16_MAX; }
};

/// Shader reflection metadata
struct ShaderReflection {
    std::string shaderId;
    std::vector<std::string> uniformNames;
    std::vector<std::string> attributeNames;
    ShaderType type = ShaderType::Unknown;
};

}  // namespace sdl3cpp::services
