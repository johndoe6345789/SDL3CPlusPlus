#include "services/interfaces/workflow/gta5/world/gta5_water.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace sdl3cpp::services::impl {
namespace {

/// `<name value="..." />` inside one item: the only shape water.xml uses.
float Value(const std::string& item, const char* name, float fallback) {
    const std::string key = std::string("<") + name + " value=\"";
    const std::size_t at = item.find(key);
    return at == std::string::npos
               ? fallback
               : std::strtof(item.c_str() + at + key.size(), nullptr);
}

}  // namespace

std::vector<Gta5WaterQuad> LoadGta5WaterQuads(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::stringstream buffer;
    buffer << in.rdbuf();
    const std::string text = buffer.str();
    const std::size_t begin = text.find("<WaterQuads>");
    const std::size_t end = text.find("</WaterQuads>");
    std::vector<Gta5WaterQuad> quads;
    if (begin == std::string::npos || end == std::string::npos) return quads;
    for (std::size_t at = text.find("<Item>", begin);
         at != std::string::npos && at < end;
         at = text.find("<Item>", at + 6)) {
        const std::size_t close = text.find("</Item>", at);
        if (close == std::string::npos) break;
        const std::string item = text.substr(at, close - at);
        if (item.find("<IsInvisible value=\"true\"") != std::string::npos) {
            continue;
        }
        Gta5WaterQuad quad;
        quad.minX = Value(item, "minX", 0.f);
        quad.maxX = Value(item, "maxX", 0.f);
        quad.minY = Value(item, "minY", 0.f);
        quad.maxY = Value(item, "maxY", 0.f);
        quad.z = Value(item, "z", 0.f);
        quad.type = static_cast<int>(Value(item, "Type", 0.f));
        quads.push_back(quad);
    }
    return quads;
}

}  // namespace sdl3cpp::services::impl
