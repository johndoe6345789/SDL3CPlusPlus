#include "services/interfaces/workflow/shader_compile_debug_marker.hpp"

#include <fstream>

namespace sdl3cpp::services::impl {

void WriteShaderCompileDebugMarker(const std::string& path,
                                   const std::string& content) {
    try {
        std::ofstream f(path);
        f << content;
        f.close();
    } catch (...) {}
}

}  // namespace sdl3cpp::services::impl
