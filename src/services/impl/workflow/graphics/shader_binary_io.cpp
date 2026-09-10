#include "services/interfaces/workflow/graphics/shader_binary_io.hpp"

#include <cstdlib>
#include <fstream>
#include <stdexcept>

namespace sdl3cpp::services::impl {

std::string ResolveShaderPath(const std::string& path) {
    if (path.empty() || path[0] != '~') return path;
    const char* home = std::getenv("HOME");
#if defined(_WIN32)
    if (!home) home = std::getenv("USERPROFILE");
#endif
    if (!home) return path;
    return std::string(home) + path.substr(1);
}

std::vector<uint8_t> LoadShaderBinary(const std::string& path) {
    const std::string resolved = ResolveShaderPath(path);
    std::ifstream file(resolved, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error(
            "graphics.gpu.shader.compile: Failed to open shader file: " +
            resolved);
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error(
            "graphics.gpu.shader.compile: Failed to read shader file: " +
            resolved);
    }
    return buffer;
}

}  // namespace sdl3cpp::services::impl
