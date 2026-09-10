#include "services/interfaces/workflow/rendering/overlay_sw_begin_resources.hpp"

#include <string>

namespace sdl3cpp::services::impl {

void PublishOverlaySwBeginTextures(const OverlaySwBeginTextures& textures,
                                   WorkflowContext& context) {
    context.Set<SDL_Texture*>("overlay.tex.bigchars", textures.bigchars);
    context.Set<SDL_Texture*>("overlay.tex.prop", textures.prop);
    context.Set<SDL_Texture*>("overlay.tex.prop_glo", textures.propGlo);
    context.Set<SDL_Texture*>("overlay.tex.frame_l", textures.frameL);
    context.Set<SDL_Texture*>("overlay.tex.frame_r", textures.frameR);
    for (int i = 0; i < 11; ++i) {
        context.Set<SDL_Texture*>("overlay.tex.num." + std::to_string(i),
                                  textures.digits[i]);
    }

    context.Set<SDL_Texture*>("overlay.tex.icon_armor", textures.iconArmor);
    context.Set<SDL_Texture*>("overlay.tex.icon_health", textures.iconHealth);
    context.Set<SDL_Texture*>("overlay.tex.icon_face", textures.iconFace);
    context.Set<SDL_Texture*>("overlay.tex.icon_weapon", textures.iconWeapon);
    context.Set<SDL_Texture*>("overlay.tex.crosshair", textures.crosshair);

    context.Set<SDL_Texture*>("overlay.tex.btn_back", textures.btnBack);
    context.Set<SDL_Texture*>("overlay.tex.btn_fight", textures.btnFight);
    context.Set<SDL_Texture*>("overlay.tex.btn_skirmish", textures.btnSkirmish);
    context.Set<SDL_Texture*>("overlay.tex.arrow_l", textures.arrowL);
    context.Set<SDL_Texture*>("overlay.tex.arrow_r", textures.arrowR);

    context.Set<std::shared_ptr<q3overlay::ArenaMap>>("overlay.arena_data",
                                                      textures.arenaData);
    context.Set<std::shared_ptr<q3overlay::LevelshotCache>>(
        "overlay.levelshot_cache", textures.levelshotCache);
}

}  // namespace sdl3cpp::services::impl
