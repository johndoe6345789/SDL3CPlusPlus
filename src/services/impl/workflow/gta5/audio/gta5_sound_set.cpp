#include "services/interfaces/workflow/gta5/audio/gta5_sound.hpp"

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

bool LoadGta5Clip(const std::filesystem::path& path, Gta5Clip& clip) {
    std::ifstream file(path, std::ios::binary);
    const std::vector<char> bytes((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());
    return ::sdl3cpp::q3::DecodeWav(
        reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), clip);
}

std::vector<Gta5Clip> LoadGta5SoundSet(
    const std::filesystem::path& folder) {
    std::vector<Gta5Clip> set;
    std::error_code error;
    std::filesystem::directory_iterator it(folder, error), end;
    for (; !error && it != end; it.increment(error)) {
        if (!IsGta5Wav(it->path())) continue;
        Gta5Clip clip;
        if (LoadGta5Clip(it->path(), clip)) set.push_back(std::move(clip));
    }
    return set;
}

}  // namespace sdl3cpp::services::impl
