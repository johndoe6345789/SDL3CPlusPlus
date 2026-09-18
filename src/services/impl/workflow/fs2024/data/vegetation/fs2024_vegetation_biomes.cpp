#include "services/interfaces/workflow/fs2024/data/vegetation/fs2024_vegetation_biomes.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "services/interfaces/workflow/fs2024/data/vegetation/fs2024_vegetation_xml.hpp"

namespace sdl3cpp::fs2024 {

std::vector<VegBiomeRule> ReadVegetationBiomeRules(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("vegetation biomes '" + path + "'");
    std::stringstream buffer;
    buffer << in.rdbuf();
    const std::string xml = buffer.str();

    std::vector<VegBiomeRule> rules;
    const std::string mark = "<BiomeRule name=\"";
    for (std::size_t at = xml.find(mark); at != std::string::npos;
         at = xml.find(mark, at + mark.size())) {
        const std::size_t nameAt = at + mark.size();
        const std::size_t nameEnd = xml.find('"', nameAt);
        const std::size_t blockEnd =
            std::min(xml.find(mark, at + mark.size()), xml.size());

        VegBiomeRule rule;
        rule.name = xml.substr(nameAt, nameEnd - nameAt);
        rule.instancesPerHectare =
            Attr(OpeningTag(xml, at), "speciesInstancesPerHectar", 0.f);
        const std::string smark = "<Species name=\"";
        for (std::size_t s = xml.find(smark, at); s != std::string::npos &&
             s < blockEnd; s = xml.find(smark, s + smark.size())) {
            const std::size_t sn = s + smark.size();
            VegBiomeSpecies species;
            species.name = xml.substr(sn, xml.find('"', sn) - sn);
            species.spawnRatio = Attr(OpeningTag(xml, s), "spawnRatio", 1.f);
            rule.species.push_back(std::move(species));
        }
        if (!rule.species.empty()) rules.push_back(std::move(rule));
    }
    if (rules.empty()) {
        throw std::runtime_error("vegetation biomes: none in " + path);
    }
    return rules;
}

}  // namespace sdl3cpp::fs2024
