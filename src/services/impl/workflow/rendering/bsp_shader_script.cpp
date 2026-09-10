#include "services/interfaces/workflow/rendering/bsp_shader_script.hpp"
#include "services/interfaces/workflow/rendering/bsp_shader_script_internal.hpp"

#include <zip.h>

namespace sdl3cpp::services::impl {

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

        bsp_shader_script_detail::ParseShaderScript(text, out);
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
