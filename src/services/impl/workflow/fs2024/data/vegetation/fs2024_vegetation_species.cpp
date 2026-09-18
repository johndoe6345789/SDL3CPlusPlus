#include "services/interfaces/workflow/fs2024/data/vegetation/fs2024_vegetation_species.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "services/interfaces/workflow/fs2024/data/vegetation/fs2024_vegetation_xml.hpp"

namespace sdl3cpp::fs2024 {
namespace {

VegVariation ReadVariation(const std::string& xml, std::size_t from,
                          std::size_t to) {
    VegVariation v;
    v.spawnRatio = Attr(OpeningTag(xml, from), "spawnRatio", 1.f);
    const auto size = xml.find("<Size", from);
    if (size != std::string::npos && size < to) {
        const std::string tag = OpeningTag(xml, size);
        v.sizeMin = Attr(tag, "min", v.sizeMin);
        v.sizeMax = Attr(tag, "max", v.sizeMax);
    }
    const auto imposter = xml.find("<Imposter", from);
    if (imposter != std::string::npos && imposter < to) {
        const std::string tag = OpeningTag(xml, imposter);
        v.frames = static_cast<int>(Attr(tag, "frames", 10.f));
        v.textureIndex = static_cast<int>(Attr(tag, "textureIndex", 0.f));
        v.relativeOffsetY = Attr(tag, "relativeOffsetY", 0.f);
    }
    return v;
}

}  // namespace

std::vector<VegSpecies> ReadVegetationSpecies(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("vegetation species '" + path + "'");
    std::stringstream buffer;
    buffer << in.rdbuf();
    const std::string xml = buffer.str();

    std::vector<VegSpecies> species;
    const std::string mark = "<Species name=\"";
    for (std::size_t at = xml.find(mark); at != std::string::npos;
         at = xml.find(mark, at + mark.size())) {
        const std::size_t nameAt = at + mark.size();
        const std::size_t nameEnd = xml.find('"', nameAt);
        const std::size_t blockEnd =
            std::min(xml.find(mark, at + mark.size()), xml.size());

        VegSpecies entry;
        entry.name = xml.substr(nameAt, nameEnd - nameAt);
        const auto material = xml.find("<Material name=\"", at);
        if (material != std::string::npos && material < blockEnd) {
            const std::size_t guidAt = material + std::string(
                "<Material name=\"").size();
            entry.materialGuid =
                xml.substr(guidAt, xml.find('"', guidAt) - guidAt);
        }
        for (std::size_t v = at;;) {
            v = xml.find("<Variation", v + 1);
            if (v == std::string::npos || v >= blockEnd) break;
            // Not "<Variations>", the wrapping element.
            if (xml[v + 10] != ' ' && xml[v + 10] != '>') continue;
            entry.variations.push_back(ReadVariation(xml, v, blockEnd));
        }
        if (!entry.variations.empty()) species.push_back(std::move(entry));
    }
    if (species.empty()) {
        throw std::runtime_error("vegetation species: none in " + path);
    }
    return species;
}

}  // namespace sdl3cpp::fs2024
