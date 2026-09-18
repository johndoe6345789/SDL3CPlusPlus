#pragma once

#include "services/interfaces/workflow/fs2024/data/vegetation/fs2024_vegetation_biomes.hpp"
#include "services/interfaces/workflow/fs2024/data/vegetation/fs2024_vegetation_species.hpp"

#include <optional>
#include <string>
#include <vector>

namespace sdl3cpp::fs2024 {

/// FS2024's own vegetation data, read once at world open: every
/// species (fs-base/vegetation/10-asobo_species.xml), every fallback
/// biome rule (00-asobo_biomes_fallback.xml), and each species'
/// material guid resolved to its own albedo texture array path (from
/// the vegetation material library's Library.xml).
struct VegetationLibrary {
    std::vector<VegSpecies> species;
    std::vector<VegBiomeRule> biomeRules;

    const VegSpecies* Species(const std::string& name) const;
    const VegBiomeRule* BiomeRule(const std::string& name) const;
    /// The species' own albedo texture array file, absolute path, or
    /// empty when its material or texture could not be found.
    std::string AlbedoPath(const VegSpecies& species) const;

    std::string materialLibraryRoot;  ///< .../Vegetation_MaterialLib
};

/// `vegetationRoot` is fs-base/vegetation; `materialLibraryRoot` is
/// fs-base-vegetation-material-lib/MaterialLibs/Vegetation_MaterialLib.
VegetationLibrary ReadVegetationLibrary(const std::string& vegetationRoot,
                                        const std::string& materialLibraryRoot);

}  // namespace sdl3cpp::fs2024
