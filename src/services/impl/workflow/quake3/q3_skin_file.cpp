#include "services/interfaces/workflow/quake3/q3_skin_file.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace sdl3cpp::q3 {
namespace {

std::string Lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return text;
}

std::string Trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

}  // namespace

SkinMap ParseSkinFile(const std::string& text) {
    SkinMap skin;
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
        const auto comma = line.find(',');
        if (comma == std::string::npos) continue;
        const std::string surface = Trim(line.substr(0, comma));
        const std::string texture = Trim(line.substr(comma + 1));
        if (surface.empty() || texture.empty()) continue;
        skin[Lower(surface)] = Lower(texture);
    }
    return skin;
}

std::string SkinTextureFor(const SkinMap& skin, const std::string& surface) {
    const auto found = skin.find(Lower(surface));
    return found == skin.end() ? std::string() : found->second;
}

}  // namespace sdl3cpp::q3
