#pragma once

#include "services/interfaces/workflow/quake3/q3_overlay_utils.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_render.h>

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Every texture (and the arena/levelshot metadata) the SW overlay
 *        draws from, loaded once from the map's pk3 and reused every
 *        frame.
 *
 * Owned by `overlay.sw.begin`, which loads it on first use and republishes
 * it into the context every frame for the `overlay.sw.*` draw steps.
 */
struct OverlaySwBeginTextures {
    SDL_Texture* bigchars = nullptr;
    SDL_Texture* prop     = nullptr;
    SDL_Texture* propGlo  = nullptr;
    SDL_Texture* frameL   = nullptr;
    SDL_Texture* frameR   = nullptr;
    SDL_Texture* digits[11] = {};
    SDL_Texture* iconArmor   = nullptr;
    SDL_Texture* iconHealth  = nullptr;
    SDL_Texture* iconFace    = nullptr;
    // iconw_machinegun — weapon icon on HUD right side.
    SDL_Texture* iconWeapon    = nullptr;
    SDL_Texture* crosshair    = nullptr;
    SDL_Texture* btnBack      = nullptr;
    SDL_Texture* btnFight     = nullptr;
    SDL_Texture* btnSkirmish  = nullptr;
    SDL_Texture* arrowL       = nullptr;
    SDL_Texture* arrowR       = nullptr;
    std::shared_ptr<q3overlay::ArenaMap> arenaData;
    std::shared_ptr<q3overlay::LevelshotCache> levelshotCache;
};

/**
 * @brief Loads every overlay texture and the arenas.txt map/bot metadata
 *        from `pk3Path` into `textures`.
 *
 * Always sets `textures.arenaData`/`levelshotCache` to fresh (possibly
 * empty) maps; individual textures are left null if their entry is
 * missing from the pk3.
 */
void LoadOverlaySwBeginTextures(SDL_Renderer* renderer,
                                const std::string& pk3Path,
                                OverlaySwBeginTextures& textures);

/// Destroys every SDL_Texture in `textures`, including cached levelshots.
void DestroyOverlaySwBeginTextures(OverlaySwBeginTextures& textures);

/// Publishes every texture and the arena/levelshot maps under their
/// "overlay.tex.*"/"overlay.arena_data"/"overlay.levelshot_cache" keys.
void PublishOverlaySwBeginTextures(const OverlaySwBeginTextures& textures,
                                   WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
