#pragma once

#include "services/interfaces/workflow/fs2024/data/bgl/fs2024_bgl_model_library.hpp"
#include "services/interfaces/workflow/fs2024/data/gltf/fs2024_gltf_model.hpp"
#include "services/interfaces/workflow/fs2024/data/texture/fs2024_dds_blocks.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// A landmark model's extent in its own space.
struct Fs2024LandmarkBounds {
    glm::vec3 min{0.f}, max{0.f};
};

/// One landmark model decoded but not yet uploaded: what a loader
/// thread hands the main thread.
struct Fs2024LandmarkMesh {
    std::string name;
    std::vector<sdl3cpp::fs2024::GltfPrimitive> primitives;
    /// Each colour map once, by its image URI; primitives name theirs
    /// with baseColorImageUri. Maps that were missing or unreadable
    /// are absent, and their primitives draw untextured.
    std::map<std::string, std::shared_ptr<const sdl3cpp::fs2024::DdsBlocks>>
        textures;
    Fs2024LandmarkBounds bounds;
};

/// The most binary glTF one landmark may load: FS2024's own LOD0 of the
/// Palace of Westminster is 3 MB, and a few city models run to tens.
constexpr std::size_t kFs2024LandmarkLodBudget = 6u << 20;

/// Reads one model from FS2024's own library BGL, CPU work only (safe
/// on any thread): the most detailed LOD whose binary glTF fits
/// `lodBudgetBytes`, with the DDS blocks of every colour map it uses
/// from `texturesDir`.
Fs2024LandmarkMesh ReadFs2024LandmarkMesh(
    const std::string& library,
    const sdl3cpp::fs2024::ModelLibraryEntry& entry,
    const std::string& texturesDir, std::size_t lodBudgetBytes);

}  // namespace sdl3cpp::services::impl
