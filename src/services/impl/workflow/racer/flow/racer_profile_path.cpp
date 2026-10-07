#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>

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

}  // namespace sdl3cpp::services::impl
