#include "services/interfaces/workflow/fs2024/data/vegetation/fs2024_vegetation_library.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace sdl3cpp::fs2024 {

const VegSpecies* VegetationLibrary::Species(const std::string& name) const {
    const auto it = std::find_if(
        species.begin(), species.end(),
        [&](const VegSpecies& s) { return s.name == name; });
    return it == species.end() ? nullptr : &*it;
}

const VegBiomeRule* VegetationLibrary::BiomeRule(
    const std::string& name) const {
    const auto it = std::find_if(
        biomeRules.begin(), biomeRules.end(),
        [&](const VegBiomeRule& r) { return r.name == name; });
    return it == biomeRules.end() ? nullptr : &*it;
}

std::string VegetationLibrary::AlbedoPath(const VegSpecies& species_) const {
    if (species_.materialGuid.empty()) return {};
    std::ifstream in(materialLibraryRoot + "/Library.xml");
    if (!in) return {};
    std::stringstream buffer;
    buffer << in.rdbuf();
    const std::string xml = buffer.str();
    const std::string guidMark = "Guid=\"" + species_.materialGuid + "\"";
    const auto material = xml.find(guidMark);
    if (material == std::string::npos) return {};
    const auto texMark = xml.find("MTL_BITMAP_DECAL0", material);
    if (texMark == std::string::npos) return {};
    const auto fileMark = xml.rfind("FileName=\"", texMark);
    if (fileMark == std::string::npos) return {};
    const auto at = fileMark + std::string("FileName=\"").size();
    std::string relative = xml.substr(at, xml.find('"', at) - at);
    for (char& c : relative) { if (c == 92) c = '/'; }
    return materialLibraryRoot + "/" + relative;
}

VegetationLibrary ReadVegetationLibrary(
    const std::string& vegetationRoot,
    const std::string& materialLibraryRoot) {
    VegetationLibrary library;
    library.materialLibraryRoot = materialLibraryRoot;
    library.species =
        ReadVegetationSpecies(vegetationRoot + "/10-asobo_species.xml");
    library.biomeRules = ReadVegetationBiomeRules(
        vegetationRoot + "/00-asobo_biomes_fallback.xml");
    return library;
}

}  // namespace sdl3cpp::fs2024
