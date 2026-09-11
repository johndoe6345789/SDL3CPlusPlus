#include "services/interfaces/workflow/gta5/gta5_lights.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace sdl3cpp::services::impl {
namespace {

/// The number after the next `key` from `at`, moving `at` past it.
float After(const std::string& text, std::size_t& at, const char* key) {
    at = text.find(key, at);
    if (at == std::string::npos) return 0.f;
    at += std::char_traits<char>::length(key);
    return std::strtof(text.c_str() + at, nullptr);
}

void ReadLights(const std::string& text, std::vector<Gta5DistantLight>& out) {
    const std::size_t soa = text.find("<DistantLODLightsSOA>");
    const std::size_t listed = text.find("</position>", soa);
    const std::size_t rgbi = text.find("<RGBI", soa);
    if (soa == std::string::npos || listed == std::string::npos ||
        rgbi == std::string::npos) {
        return;
    }
    const char* colours = text.c_str() + text.find('>', rgbi) + 1;
    for (std::size_t at = soa;;) {
        std::size_t item = text.find("<x value=\"", at);
        if (item == std::string::npos || item > listed) break;
        const float x = After(text, item, "<x value=\"");
        const float y = After(text, item, "<y value=\"");
        const float z = After(text, item, "<z value=\"");
        at = item;
        char* end = nullptr;
        const unsigned long v = std::strtoul(colours, &end, 10);
        if (end == colours) break;  // fewer colours than positions
        colours = end;
        Gta5DistantLight light;
        light.position = glm::vec3(x, z, -y);  // GTA to engine
        light.colour = glm::vec3((v >> 16) & 255u, (v >> 8) & 255u,
                                 v & 255u) / 255.f;
        light.intensity = static_cast<float>((v >> 24) & 255u) / 255.f;
        out.push_back(light);
    }
}

}  // namespace

std::vector<Gta5DistantLight> LoadGta5DistantLights(const std::string& dir) {
    namespace fs = std::filesystem;
    std::vector<Gta5DistantLight> lights;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        const std::string name = entry.path().filename().string();
        if (name.rfind("distlodlights_", 0) != 0 ||
            name.size() < 9 || name.substr(name.size() - 9) != ".ymap.xml") {
            continue;
        }
        std::ifstream in(entry.path(), std::ios::binary);
        std::stringstream text;
        text << in.rdbuf();
        ReadLights(text.str(), lights);
    }
    return lights;
}

}  // namespace sdl3cpp::services::impl
