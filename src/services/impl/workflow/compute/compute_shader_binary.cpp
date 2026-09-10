#include "services/interfaces/workflow/compute/compute_shader_binary.hpp"

#include <cstdlib>
#include <fstream>
#include <stdexcept>

namespace sdl3cpp::services::impl {

std::string ExpandComputeShaderPath(const std::string& path) {
    if (path.empty() || path[0] != '~') {
        return path;
    }
    const char* home = std::getenv("HOME");
    return home ? std::string(home) + path.substr(1) : path;
}

std::vector<uint8_t> LoadComputeShaderBinary(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open compute shader: " + path);
    }
    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> data(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

}  // namespace sdl3cpp::services::impl
