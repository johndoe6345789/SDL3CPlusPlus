#pragma once

#include "services/interfaces/workflow/racer/data/racer_texture.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// One vertex of a decoded model, in the game's own z-up space.
struct RacerModelVertex {
    float x = 0.f, y = 0.f, z = 0.f;
    float u = 0.f, v = 0.f;
    std::uint8_t r = 255, g = 255, b = 255, a = 255;
};

/// The texture a batch samples: an index into the texture block plus
/// the format and size its material declares. `doubleWidth` and
/// `doubleHeight` mean the image is mirrored to twice its size before
/// the uvs (which run 0..1 over the mirrored image) apply.
struct RacerMaterialRef {
    int textureIndex = -1;
    RacerTextureFormat format = RacerTextureFormat::Indexed4;
    int width = 0;
    int height = 0;
    bool doubleWidth = false;
    bool doubleHeight = false;

    bool operator==(const RacerMaterialRef& other) const;
};

/// Triangles sharing one material, as a flat triangle list.
struct RacerModelBatch {
    RacerMaterialRef material;
    std::vector<RacerModelVertex> vertices;
};

/// A model item flattened to world space: every reachable mesh with its
/// node transforms applied, grouped by material.
struct RacerModel {
    std::uint32_t tag = 0;   ///< 'Trak', 'Podd', 'Part', ... big-endian.
    std::vector<RacerModelBatch> batches;
    /// The invisible collision surface pods ride on and bounce off:
    /// x, y, z per corner, three corners per triangle, game space.
    std::vector<float> collision;
    /// The surface's vehicle-reaction flags, one per collision triangle
    /// (see RacerSurfaceFlag).
    std::vector<std::uint32_t> collisionFlags;
    int meshCount = 0;
    int triangleCount = 0;
    bool valid = false;
};

/// What to keep while walking a model's node tree.
enum class RacerModelScope {
    /// Every reachable mesh, with every node transform applied. Right
    /// for tracks and scenery.
    Everything,
    /// A pod: only its parts (two engines and a cockpit, each under a
    /// 0.02 scale node the game replaces at run time), each from that
    /// node down, laid out engines ahead and cockpit behind. The shadow
    /// quads and afterburner cones around them are left out. The game
    /// spaces the parts from per-racer data not yet decoded, so the
    /// layout here is an approximation.
    PodParts,
};

/// Parses the data part of a model block item (see the racer README for
/// the layout). Pointers inside the item are offsets from its start.
RacerModel ParseRacerModel(const std::vector<std::uint8_t>& data,
                           RacerModelScope scope = RacerModelScope::Everything);

}  // namespace sdl3cpp::services::impl
