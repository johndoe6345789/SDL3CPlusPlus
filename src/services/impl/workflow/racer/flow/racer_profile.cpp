#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>

namespace sdl3cpp::services::impl {

std::filesystem::path RacerProfilePath() {
    // RACER_PROFILE names another file (tests, or several players).
    if (const char* override = SDL_getenv("RACER_PROFILE")) {
        if (override[0] != 0) return override;
    }
    char* folder = SDL_GetPrefPath("SDL3CPlusPlus", "EpisodeIRacer");
    if (!folder) return "racer_profile.json";
    std::filesystem::path path = std::filesystem::path(folder) /
                                 "profile.json";
    SDL_free(folder);
    return path;
}

RacerProfile LoadRacerProfile(const std::filesystem::path& path) {
    RacerProfile profile;
    std::ifstream in(path);
    if (!in) return profile;
    const auto doc = nlohmann::json::parse(in, nullptr, false);
    if (doc.is_discarded()) return profile;
    profile.truguts = std::max(0, doc.value("truguts", 0));
    const auto levels = doc.value("upgrades", nlohmann::json::array());
    for (int i = 0; i < kRacerUpgradeCount &&
                    i < static_cast<int>(levels.size());
         ++i) {
        profile.upgrades[i] =
            std::clamp(levels[i].get<int>(), 0, kRacerUpgradeMax);
    }
    return profile;
}

bool SaveRacerProfile(const RacerProfile& profile,
                      const std::filesystem::path& path) {
    nlohmann::json doc;
    doc["truguts"] = profile.truguts;
    doc["upgrades"] = profile.upgrades;
    std::ofstream out(path);
    if (!out) return false;
    out << doc.dump(1) << '\n';
    return static_cast<bool>(out);
}

}  // namespace sdl3cpp::services::impl
