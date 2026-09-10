#include "services/interfaces/workflow/rendering/overlay_sw_begin_resources.hpp"

#include <zip.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

using q3overlay::ArenaMap;
using q3overlay::LevelshotCache;
using q3overlay::LoadTextureFromPk3;

// Parses scripts/arenas.txt out of the map's pk3 into a lowercase-keyed
// map name -> {longname, first bot} table for the map-select menu.
ArenaMap ParseArenasTxt(const std::string& pk3) {
    ArenaMap arenaData;
    int ze = 0;
    zip_t* arc = zip_open(pk3.c_str(), ZIP_RDONLY, &ze);
    if (!arc) return arenaData;

    zip_stat_t st;
    if (zip_stat(arc, "scripts/arenas.txt", 0, &st) == 0) {
        std::vector<char> buf(st.size + 1, '\0');
        zip_file_t* zf = zip_fopen(arc, "scripts/arenas.txt", 0);
        if (zf) {
            zip_fread(zf, buf.data(), st.size);
            zip_fclose(zf);
        }
        std::string text(buf.data());
        size_t pos = 0;
        while ((pos = text.find('{', pos)) != std::string::npos) {
            size_t end = text.find('}', pos);
            if (end == std::string::npos) break;
            std::string block = text.substr(pos + 1, end - pos - 1);
            pos = end + 1;
            std::string mapName, longName, bots;
            std::istringstream ss(block);
            std::string tok;
            while (ss >> tok) {
                auto readVal = [&]() -> std::string {
                    std::string v;
                    ss >> std::ws;
                    if (ss.peek() == '"') {
                        ss.get();
                        std::getline(ss, v, '"');
                    } else {
                        ss >> v;
                    }
                    return v;
                };
                if (tok == "map") mapName = readVal();
                else if (tok == "longname") longName = readVal();
                else if (tok == "bots") bots = readVal();
            }
            if (!mapName.empty()) {
                std::string key = mapName;
                std::transform(key.begin(), key.end(), key.begin(),
                               ::tolower);
                std::string firstBot = bots.substr(0, bots.find(' '));
                arenaData[key] = {longName, firstBot};
            }
        }
    }
    zip_close(arc);
    return arenaData;
}

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
        "gfx/2d/numbers/minus_32b.tga"
    };
    for (int i = 0; i < 11; ++i) textures.digits[i] = load(kDigits[i]);

    // weapon icon (right HUD), not ammo pickup.
    textures.iconArmor  = load("icons/iconr_yellow.tga");
    textures.iconHealth = load("icons/iconh_red.tga");
    textures.iconFace   = load("models/players/keel/icon_default.tga");
    textures.iconWeapon = load("icons/iconw_machinegun.tga");
    textures.crosshair  = load("gfx/2d/crosshaira.tga");
    textures.btnBack     = load("menu/art/back_0.tga");
    textures.btnFight    = load("menu/art/fight_0.tga");
    textures.btnSkirmish = load("menu/art/skirmish_0.tga");
    textures.arrowL      = load("menu/art/gs_arrows_l.tga");
    textures.arrowR      = load("menu/art/gs_arrows_r.tga");

    textures.arenaData = std::make_shared<ArenaMap>(ParseArenasTxt(pk3Path));
    textures.levelshotCache = std::make_shared<LevelshotCache>();
}

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
    for (auto& d : textures.digits) destroy(d);
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
    context.Set<SDL_Texture*>("overlay.tex.icon_health",
                              textures.iconHealth);
    context.Set<SDL_Texture*>("overlay.tex.icon_face", textures.iconFace);
    context.Set<SDL_Texture*>("overlay.tex.icon_weapon",
                              textures.iconWeapon);
    context.Set<SDL_Texture*>("overlay.tex.crosshair", textures.crosshair);

    context.Set<SDL_Texture*>("overlay.tex.btn_back", textures.btnBack);
    context.Set<SDL_Texture*>("overlay.tex.btn_fight", textures.btnFight);
    context.Set<SDL_Texture*>("overlay.tex.btn_skirmish",
                              textures.btnSkirmish);
    context.Set<SDL_Texture*>("overlay.tex.arrow_l", textures.arrowL);
    context.Set<SDL_Texture*>("overlay.tex.arrow_r", textures.arrowR);

    context.Set<std::shared_ptr<q3overlay::ArenaMap>>(
        "overlay.arena_data", textures.arenaData);
    context.Set<std::shared_ptr<q3overlay::LevelshotCache>>(
        "overlay.levelshot_cache", textures.levelshotCache);
}

}  // namespace sdl3cpp::services::impl
