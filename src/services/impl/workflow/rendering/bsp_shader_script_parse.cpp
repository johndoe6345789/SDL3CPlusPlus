#include "services/interfaces/workflow/rendering/bsp_shader_script_internal.hpp"

#include <algorithm>
#include <sstream>

namespace sdl3cpp::services::impl::bsp_shader_script_detail {

void ParseShaderScript(const std::string& text,
                       std::map<std::string, std::string>& out) {
    std::string shader;
    int depth = 0;
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
        const auto comment = line.find("//");
        if (comment != std::string::npos) {
            line.erase(comment);
        }
        std::istringstream words(line);
        std::string word;
        if (!(words >> word)) {
            continue;
        }
        if (word == "{") {
            ++depth;
            continue;
        }
        if (word == "}") {
            if (depth > 0) --depth;
            continue;
        }
        if (depth == 0) {
            shader = word;
            continue;
        }
        if (shader.empty() || out.count(shader)) {
            continue;
        }

        std::string lowered = word;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (lowered != "map" && lowered != "clampmap") {
            continue;
        }

        std::string image;
        if (!(words >> image) || image.empty() || image[0] == '$') {
            continue;
        }
        const auto dot = image.rfind('.');
        if (dot != std::string::npos) {
            image.erase(dot);
        }
        out[shader] = image;
    }
}

}  // namespace sdl3cpp::services::impl::bsp_shader_script_detail
