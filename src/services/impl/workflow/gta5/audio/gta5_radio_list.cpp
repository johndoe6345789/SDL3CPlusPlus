#include "services/interfaces/workflow/gta5/audio/gta5_radio.hpp"

#include <SDL3/SDL_audio.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <system_error>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

std::vector<std::filesystem::path> Tracks(const std::filesystem::path& at) {
    std::vector<std::filesystem::path> tracks;
    std::error_code error;
    std::filesystem::recursive_directory_iterator it(at, error), end;
    for (; !error && it != end; it.increment(error)) {
        if (IsGta5Wav(it->path())) tracks.push_back(it->path());
    }
    return tracks;
}

}  // namespace

std::vector<Gta5Station> ListGta5Stations(const std::filesystem::path& dir,
                                          const std::string& namesFile) {
    std::ifstream in(namesFile);
    const nlohmann::json names = nlohmann::json::parse(in, nullptr, false);
    std::vector<Gta5Station> stations;
    std::error_code error;
    std::filesystem::directory_iterator it(dir, error), end;
    for (; !error && it != end; it.increment(error)) {
        std::error_code kind;
        if (!it->is_directory(kind)) continue;
        Gta5Station station;
        station.tracks = Tracks(it->path());
        if (station.tracks.empty()) continue;
        std::string folder = it->path().filename().string();
        station.name = folder;
        std::transform(folder.begin(), folder.end(), folder.begin(),
                       [](unsigned char c) { return char(std::toupper(c)); });
        if (names.is_object()) station.name = names.value(folder, station.name);
        stations.push_back(std::move(station));
    }
    return stations;
}

Gta5Clip LoadGta5Track(const std::filesystem::path& path) {
    Gta5Clip clip;
    Uint8* pcm = nullptr;
    Uint32 length = 0;
    if (!SDL_LoadWAV(path.string().c_str(), &clip.spec, &pcm, &length)) {
        return clip;
    }
    clip.pcm.assign(pcm, pcm + length);
    SDL_free(pcm);
    return clip;
}

}  // namespace sdl3cpp::services::impl
