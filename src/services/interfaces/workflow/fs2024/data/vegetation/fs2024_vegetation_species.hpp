#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::fs2024 {

/// One tree size/imposter variant of a species, from FS2024's own
/// vegetation/10-asobo_species.xml.
struct VegVariation {
    float sizeMin = 10.f, sizeMax = 20.f;  ///< tree height, metres
    int frames = 10;         ///< the atlas is a frames x frames grid
    int textureIndex = 0;    ///< which array layer this variant samples
    float relativeOffsetY = 0.f;  ///< sinks the billboard, a fraction of size
    float spawnRatio = 1.f;
};

/// One species: a name (used by the biome rules), its size/imposter
/// variants, and the vegetation material library guid whose albedo
/// texture array the variants' `textureIndex` picks a layer from.
struct VegSpecies {
    std::string name;
    std::vector<VegVariation> variations;
    std::string materialGuid;  ///< "{...}", matches Library.xml's Guid
};

/// Parses FS2024's own species library (fs-base/vegetation/
/// 10-asobo_species.xml): every `<Species>` with its `<Variations>`
/// and the material guid it points at.
std::vector<VegSpecies> ReadVegetationSpecies(const std::string& path);

}  // namespace sdl3cpp::fs2024
