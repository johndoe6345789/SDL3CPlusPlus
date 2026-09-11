#include "services/interfaces/workflow/gta5/gta5_sound.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <system_error>
#include <utility>

namespace sdl3cpp::services::impl {

bool IsGta5Wav(const std::filesystem::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return char(std::tolower(c)); });
    return ext == ".wav";
}

namespace {

std::vector<Gta5Clip> LoadSet(const std::filesystem::path& folder) {
    std::vector<Gta5Clip> set;
    std::error_code error;
    std::filesystem::directory_iterator it(folder, error), end;
    for (; !error && it != end; it.increment(error)) {
        if (!IsGta5Wav(it->path())) continue;
        std::ifstream file(it->path(), std::ios::binary);
        const std::vector<char> bytes((std::istreambuf_iterator<char>(file)),
                                      std::istreambuf_iterator<char>());
        Gta5Clip clip;
        if (::sdl3cpp::q3::DecodeWav(
                reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(),
                clip)) {
            set.push_back(std::move(clip));
        }
    }
    return set;
}

}  // namespace

Gta5Sounds LoadGta5Sounds(const std::filesystem::path& dir,
                          const std::shared_ptr<ILogger>& logger) {
    Gta5Sounds sounds;
    const std::pair<const char*, std::vector<Gta5Clip>*> kinds[] = {
        {"steps", &sounds.steps},   {"strokes", &sounds.strokes},
        {"splash", &sounds.splash}, {"engine", &sounds.engine},
        {"water", &sounds.water}};
    std::string report;
    for (const auto& [name, set] : kinds) {
        *set = LoadSet(dir / name);
        report += std::string(" ") + name + " ";
        if (!set->empty()) {
            report += std::to_string(set->size());
            continue;
        }
        // Stand-ins until GTA's are exported: four of a one-shot, so
        // they vary; one of a loop.
        const bool loop = set == &sounds.engine || set == &sounds.water;
        for (int v = 0; v < (loop ? 1 : 4); ++v) {
            set->push_back(SynthGta5Sound(name, v));
        }
        report += "synth";
    }
    if (logger) {
        logger->Info("gta5.sound: from " + dir.string() + ":" + report);
    }
    return sounds;
}

}  // namespace sdl3cpp::services::impl
