#pragma once

#include "services/interfaces/workflow/fs2024/build/fs2024_class_sampler.hpp"
#include "services/interfaces/workflow/fs2024/build/fs2024_dem_sampler.hpp"
#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_library.hpp"
#include "services/interfaces/workflow/fs2024/data/landmark/fs2024_landmark_index.hpp"
#include "services/interfaces/workflow/fs2024/data/material/fs2024_ground_materials.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_geo_origin.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services::impl {

/// Where the game is installed: the folders FS2024 keeps its world in.
struct Fs2024InstallPaths {
    std::string cglRoot;       ///< fs-base-cgl (dem, lcg, bld, vec, ...)
    std::string texSynthRoot;  ///< bf-texture-synth-lib/.../BFTexSynthLib
    std::string pggRoot;       ///< bf-pgg/PGG (building generator data)
    /// FS2024's own landmark library (Asobo_POI.BGL) and its textures;
    /// empty when the install has none.
    std::string landmarkLibrary;
    std::string landmarkTextures;
    /// The worldwide object grid whose placements say where they stand.
    std::string landmarkScenery;
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
    /// Every landmark FS2024 places, by the level-14 quad it stands in
    /// (Fs2024QuadId).
    std::unordered_map<std::uint64_t,
                       std::vector<sdl3cpp::fs2024::LandmarkPlacement>>
        landmarks;
};

/// A level-14 quad tile's key in Fs2024World::landmarks.
std::uint64_t Fs2024QuadId(int quadX, int quadY);

/// Buckets FS2024's own landmark placements into `world.landmarks`.
void BucketFs2024Landmarks(
    Fs2024World& world,
    const std::vector<sdl3cpp::fs2024::LandmarkPlacement>& placements);

}  // namespace sdl3cpp::services::impl
