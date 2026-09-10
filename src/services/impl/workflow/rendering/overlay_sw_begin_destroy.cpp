#include "services/interfaces/workflow/rendering/overlay_sw_begin_resources.hpp"

namespace sdl3cpp::services::impl {

void DestroyOverlaySwBeginTextures(OverlaySwBeginTextures& textures) {
    auto destroy = [](SDL_Texture*& t) {
        if (t) {
            SDL_DestroyTexture(t);
            t = nullptr;
        }
    };
    destroy(textures.bigchars);
    destroy(textures.prop);
    destroy(textures.propGlo);
    destroy(textures.frameL);
    destroy(textures.frameR);
    for (auto& d : textures.digits)
        destroy(d);
    destroy(textures.iconArmor);
    destroy(textures.iconHealth);
    destroy(textures.iconFace);
    destroy(textures.iconWeapon);
    destroy(textures.crosshair);
    destroy(textures.btnBack);
    destroy(textures.btnFight);
    destroy(textures.btnSkirmish);
    destroy(textures.arrowL);
    destroy(textures.arrowR);
    // Levelshot textures owned by the cache.
    if (textures.levelshotCache) {
        for (auto& [k, v] : *textures.levelshotCache) {
            if (v) SDL_DestroyTexture(v);
        }
    }
}

}  // namespace sdl3cpp::services::impl
