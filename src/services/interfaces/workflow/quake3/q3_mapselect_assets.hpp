#pragma once

#include "services/interfaces/workflow/quake3/q3_overlay_utils.hpp"

#include <SDL3/SDL_render.h>
#include <nlohmann/json.hpp>

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Everything the map-select screen needs from context, gathered up
/// front so the drawing code below doesn't touch WorkflowContext itself.
struct Q3MapSelectAssets {
    SDL_Renderer* renderer   = nullptr;
    SDL_Texture* prop        = nullptr;
    SDL_Texture* bigchars    = nullptr;
    SDL_Texture* arrowLeft   = nullptr;
    SDL_Texture* arrowRight  = nullptr;
    SDL_Texture* btnBack     = nullptr;
    SDL_Texture* btnFight    = nullptr;
    SDL_Texture* btnSkirmish = nullptr;
    std::shared_ptr<q3overlay::ArenaMap> arenas;
    std::shared_ptr<q3overlay::LevelshotCache> shots;
    std::string pk3Path;
    nlohmann::json maps;
    int selectedItem = 0;
};

}  // namespace sdl3cpp::services::impl
