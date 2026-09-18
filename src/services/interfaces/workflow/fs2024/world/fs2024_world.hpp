#pragma once

#include "services/interfaces/workflow/fs2024/build/fs2024_class_sampler.hpp"
#include "services/interfaces/workflow/fs2024/build/fs2024_dem_sampler.hpp"
#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_library.hpp"
#include "services/interfaces/workflow/fs2024/data/material/fs2024_ground_materials.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_geo_origin.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <array>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Where the game is installed: the folders FS2024 keeps its world in.
struct Fs2024InstallPaths {
    std::string cglRoot;       ///< fs-base-cgl (dem, lcg, bld, vec, ...)
    std::string texSynthRoot;  ///< bf-texture-synth-lib/.../BFTexSynthLib
    std::string pggRoot;       ///< bf-pgg/PGG (building generator data)
};

/// The paths under an install root (the folder holding FS2024's
/// packages, e.g. `.../Official/Steam`). Throws naming whichever is
/// missing.
Fs2024InstallPaths ResolveFs2024InstallPaths(const std::string& installRoot);

/// How many land classes the ground shader's material table covers.
constexpr int kFs2024LandClasses = 16;

/// The world the streaming tiles are cut from: where engine space sits
/// on the Earth, and FS2024's own data behind every tile -- opened once
/// by fs2024.world.open and read by every tile load after.
struct Fs2024World {
    Fs2024InstallPaths paths;
    Fs2024GeoOrigin origin;
    float spawnX = 0.f, spawnZ = 0.f, spawnHeading = 0.f;
    std::unique_ptr<Fs2024DemSampler> dem;
    std::unique_ptr<Fs2024ClassSampler> classes;
    std::unique_ptr<sdl3cpp::fs2024::BldLibrary> buildings;
    sdl3cpp::fs2024::Fs2024GroundMaterials materials;
    /// FS2024's ground material array on the GPU, as-is: 653 layers of
    /// 256 x 256 BC1 sRGB (autogen/array_low.dds).
    SDL_GPUTexture* materialArray = nullptr;
    SDL_GPUSampler* materialSampler = nullptr;
    /// Per land class: x = first layer, y = layer count.
    std::array<glm::vec4, kFs2024LandClasses> materialTable{};
};

}  // namespace sdl3cpp::services::impl
