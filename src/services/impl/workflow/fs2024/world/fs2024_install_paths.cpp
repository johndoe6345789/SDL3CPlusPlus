#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"

#include <filesystem>
#include <stdexcept>

namespace sdl3cpp::services::impl {
namespace {

std::string Required(const std::string& path, const char* what) {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error(std::string("FS2024 install: no ") + what +
                                 " at '" + path + "'");
    }
    return path;
}

}  // namespace

Fs2024InstallPaths ResolveFs2024InstallPaths(const std::string& installRoot) {
    Fs2024InstallPaths paths;
    paths.cglRoot = Required(installRoot + "/fs-base-cgl", "world data");
    paths.texSynthRoot =
        Required(installRoot +
                     "/bf-texture-synth-lib/TexSynthLibs/BFTexSynthLib",
                 "ground materials");
    paths.pggRoot = Required(installRoot + "/bf-pgg/PGG", "building data");
    return paths;
}

}  // namespace sdl3cpp::services::impl
