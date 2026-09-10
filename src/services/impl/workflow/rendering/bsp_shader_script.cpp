#include "services/interfaces/workflow/rendering/bsp_shader_script.hpp"

#include <zip.h>

#include <algorithm>
#include <sstream>

namespace sdl3cpp::services::impl {
namespace {

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

}  // namespace

std::map<std::string, std::string> LoadShaderImages(zip_t* archive) {
    std::map<std::string, std::string> out;
    const zip_int64_t count = zip_get_num_entries(archive, 0);
    for (zip_int64_t i = 0; i < count; ++i) {
        const char* raw = zip_get_name(archive, i, 0);
        if (!raw) {
            continue;
        }
        const std::string entry(raw);
        if (entry.rfind("scripts/", 0) != 0) {
            continue;
        }
        if (entry.size() < 7 ||
            entry.compare(entry.size() - 7, 7, ".shader") != 0) {
            continue;
        }

        zip_stat_t st;
        if (zip_stat_index(archive, i, 0, &st) != 0) {
            continue;
        }
        zip_file_t* file = zip_fopen_index(archive, i, 0);
        if (!file) {
            continue;
        }
        std::string text(static_cast<size_t>(st.size), '\0');
        zip_fread(file, text.data(), st.size);
        zip_fclose(file);

        ParseShaderScript(text, out);
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
