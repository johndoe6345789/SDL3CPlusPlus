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
    const std::string poi =
        installRoot + "/fs-base/scenery/Global/Asobo_POI";
    const std::string scenery =
        installRoot + "/fs-base-genericairports/scenery";
    if (std::filesystem::exists(poi + "/Asobo_POI.BGL") &&
        std::filesystem::exists(scenery)) {
        paths.landmarkLibrary = poi + "/Asobo_POI.BGL";
        paths.landmarkTextures = poi + "/TEXTURE";
        paths.landmarkScenery = scenery;
    }
    const std::string vegetation = installRoot + "/fs-base/vegetation";
    const std::string vegetationLib =
        installRoot +
        "/fs-base-vegetation-material-lib/MaterialLibs/Vegetation_MaterialLib";
    if (std::filesystem::exists(vegetation) &&
        std::filesystem::exists(vegetationLib)) {
        paths.vegetationRoot = vegetation;
        paths.vegetationMaterialRoot = vegetationLib;
    }
    return paths;
}

}  // namespace sdl3cpp::services::impl
