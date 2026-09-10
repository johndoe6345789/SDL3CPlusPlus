#include "services/interfaces/workflow/rendering/overlay_arenas_txt_parser.hpp"
#include "services/interfaces/workflow/rendering/overlay_sw_begin_resources.hpp"

namespace sdl3cpp::services::impl {
namespace {

using q3overlay::ArenaMap;
using q3overlay::LevelshotCache;
using q3overlay::LoadTextureFromPk3;

}  // namespace

void LoadOverlaySwBeginTextures(SDL_Renderer* renderer,
                                const std::string& pk3Path,
                                OverlaySwBeginTextures& textures) {
    auto load = [&](const char* e) {
        return LoadTextureFromPk3(renderer, pk3Path, e);
    };

    textures.bigchars = load("gfx/2d/bigchars.tga");
    textures.prop     = load("menu/art/font1_prop.tga");
    textures.propGlo  = load("menu/art/font1_prop_glo.tga");
    textures.frameL   = load("menu/art/frame1_l.tga");
    textures.frameR   = load("menu/art/frame1_r.tga");

    static const char* kDigits[11] = {
        "gfx/2d/numbers/zero_32b.tga",  "gfx/2d/numbers/one_32b.tga",
        "gfx/2d/numbers/two_32b.tga",   "gfx/2d/numbers/three_32b.tga",
        "gfx/2d/numbers/four_32b.tga",  "gfx/2d/numbers/five_32b.tga",
        "gfx/2d/numbers/six_32b.tga",   "gfx/2d/numbers/seven_32b.tga",
        "gfx/2d/numbers/eight_32b.tga", "gfx/2d/numbers/nine_32b.tga",
        "gfx/2d/numbers/minus_32b.tga"};
    for (int i = 0; i < 11; ++i)
        textures.digits[i] = load(kDigits[i]);

    // weapon icon (right HUD), not ammo pickup.
    textures.iconArmor   = load("icons/iconr_yellow.tga");
    textures.iconHealth  = load("icons/iconh_red.tga");
    textures.iconFace    = load("models/players/keel/icon_default.tga");
    textures.iconWeapon  = load("icons/iconw_machinegun.tga");
    textures.crosshair   = load("gfx/2d/crosshaira.tga");
    textures.btnBack     = load("menu/art/back_0.tga");
    textures.btnFight    = load("menu/art/fight_0.tga");
    textures.btnSkirmish = load("menu/art/skirmish_0.tga");
    textures.arrowL      = load("menu/art/gs_arrows_l.tga");
    textures.arrowR      = load("menu/art/gs_arrows_r.tga");

    textures.arenaData = std::make_shared<ArenaMap>(ParseArenasTxt(pk3Path));
    textures.levelshotCache = std::make_shared<LevelshotCache>();
}

}  // namespace sdl3cpp::services::impl
