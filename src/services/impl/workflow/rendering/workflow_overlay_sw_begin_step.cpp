#include "services/interfaces/workflow/rendering/workflow_overlay_sw_begin_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include <nlohmann/json.hpp>
#include <SDL3/SDL.h>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {
using namespace q3overlay;

WorkflowOverlaySwBeginStep::WorkflowOverlaySwBeginStep(std::shared_ptr<ILogger> l)
    : logger_(std::move(l)) {}

WorkflowOverlaySwBeginStep::~WorkflowOverlaySwBeginStep() {
    if (renderer_) {
        auto destroy = [&](SDL_Texture*& t) { if (t) { SDL_DestroyTexture(t); t=nullptr; } };
        destroy(bigchars_); destroy(prop_); destroy(prop_glo_);
        destroy(frame_l_); destroy(frame_r_);
        for (auto& d : digits_) destroy(d);
        destroy(icon_armor_); destroy(icon_health_); destroy(icon_face_); destroy(icon_weapon_);
        destroy(crosshair_); destroy(btn_back_); destroy(btn_fight_); destroy(btn_skirmish_);
        destroy(arrow_l_); destroy(arrow_r_);
        // Levelshot textures owned by the cache
        if (levelshot_cache_) {
            for (auto& [k,v] : *levelshot_cache_) if (v) SDL_DestroyTexture(v);
        }
        SDL_DestroyRenderer(renderer_);
    }
    if (surface_) SDL_DestroySurface(surface_);
}

std::string WorkflowOverlaySwBeginStep::GetPluginId() const { return "overlay.sw.begin"; }

void WorkflowOverlaySwBeginStep::TryLoadTextures(const std::string& pk3) {
    if (tex_loaded_) return;
    tex_loaded_ = true;

    auto load = [&](const char* e) { return LoadTextureFromPk3(renderer_, pk3, e); };

    bigchars_  = load("gfx/2d/bigchars.tga");
    prop_      = load("menu/art/font1_prop.tga");
    prop_glo_  = load("menu/art/font1_prop_glo.tga");
    frame_l_   = load("menu/art/frame1_l.tga");
    frame_r_   = load("menu/art/frame1_r.tga");

    static const char* kDigits[11] = {
        "gfx/2d/numbers/zero_32b.tga",  "gfx/2d/numbers/one_32b.tga",
        "gfx/2d/numbers/two_32b.tga",   "gfx/2d/numbers/three_32b.tga",
        "gfx/2d/numbers/four_32b.tga",  "gfx/2d/numbers/five_32b.tga",
        "gfx/2d/numbers/six_32b.tga",   "gfx/2d/numbers/seven_32b.tga",
        "gfx/2d/numbers/eight_32b.tga", "gfx/2d/numbers/nine_32b.tga",
        "gfx/2d/numbers/minus_32b.tga"
    };
    for (int i = 0; i < 11; ++i) digits_[i] = load(kDigits[i]);

    icon_armor_   = load("icons/iconr_yellow.tga");
    icon_health_  = load("icons/iconh_red.tga");
    icon_face_    = load("models/players/keel/icon_default.tga");
    icon_weapon_  = load("icons/iconw_machinegun.tga");  // weapon icon (right HUD), not ammo pickup
    crosshair_    = load("gfx/2d/crosshaira.tga");
    btn_back_     = load("menu/art/back_0.tga");
    btn_fight_    = load("menu/art/fight_0.tga");
    btn_skirmish_ = load("menu/art/skirmish_0.tga");
    arrow_l_      = load("menu/art/gs_arrows_l.tga");
    arrow_r_      = load("menu/art/gs_arrows_r.tga");

    // Parse arenas.txt
    arena_data_ = std::make_shared<ArenaMap>();
    levelshot_cache_ = std::make_shared<LevelshotCache>();
    int ze = 0;
    zip_t* arc = zip_open(pk3.c_str(), ZIP_RDONLY, &ze);
    if (arc) {
        zip_stat_t st;
        if (zip_stat(arc, "scripts/arenas.txt", 0, &st) == 0) {
            std::vector<char> buf(st.size + 1, '\0');
            zip_file_t* zf = zip_fopen(arc, "scripts/arenas.txt", 0);
            if (zf) { zip_fread(zf, buf.data(), st.size); zip_fclose(zf); }
            std::string text(buf.data());
            size_t pos = 0;
            while ((pos = text.find('{', pos)) != std::string::npos) {
                size_t end = text.find('}', pos);
                if (end == std::string::npos) break;
                std::string block = text.substr(pos+1, end-pos-1);
                pos = end + 1;
                std::string mapName, longName, bots;
                std::istringstream ss(block);
                std::string tok;
                while (ss >> tok) {
                    auto readVal = [&]() -> std::string {
                        std::string v; ss >> std::ws;
                        if (ss.peek()=='"') { ss.get(); std::getline(ss,v,'"'); }
                        else { ss >> v; }
                        return v;
                    };
                    if (tok=="map") mapName = readVal();
                    else if (tok=="longname") longName = readVal();
                    else if (tok=="bots") bots = readVal();
                }
                if (!mapName.empty()) {
                    std::string key = mapName;
                    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                    std::string firstBot = bots.substr(0, bots.find(' '));
                    (*arena_data_)[key] = {longName, firstBot};
                }
            }
        }
        zip_close(arc);
    }

    if (logger_) logger_->Info("overlay.sw.begin: textures loaded from " + pk3);
}

void WorkflowOverlaySwBeginStep::Execute(
        const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    if (!ready_) {
        surface_  = SDL_CreateSurface(kW, kH, SDL_PIXELFORMAT_RGBA32);
        renderer_ = surface_ ? SDL_CreateSoftwareRenderer(surface_) : nullptr;
        ready_    = surface_ && renderer_;
    }
    if (!ready_) return;

    const auto bspCfg = context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    const std::string pk3 = bspCfg.value("pk3_path", std::string(""));
    if (!tex_loaded_ && !pk3.empty()) TryLoadTextures(pk3);

    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 0);
    SDL_RenderClear(renderer_);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    // Expose surface/renderer so drawing steps can use them
    context.Set<SDL_Renderer*>("overlay.renderer", renderer_);
    context.Set<SDL_Surface*> ("overlay.surface",  surface_);
    context.Set<bool>         ("overlay.ready",    ready_);

    // Font textures
    context.Set<SDL_Texture*>("overlay.tex.bigchars",  bigchars_);
    context.Set<SDL_Texture*>("overlay.tex.prop",      prop_);
    context.Set<SDL_Texture*>("overlay.tex.prop_glo",  prop_glo_);
    context.Set<SDL_Texture*>("overlay.tex.frame_l",   frame_l_);
    context.Set<SDL_Texture*>("overlay.tex.frame_r",   frame_r_);
    for (int i = 0; i < 11; ++i)
        context.Set<SDL_Texture*>("overlay.tex.num." + std::to_string(i), digits_[i]);

    // HUD icons
    context.Set<SDL_Texture*>("overlay.tex.icon_armor",   icon_armor_);
    context.Set<SDL_Texture*>("overlay.tex.icon_health",  icon_health_);
    context.Set<SDL_Texture*>("overlay.tex.icon_face",    icon_face_);
    context.Set<SDL_Texture*>("overlay.tex.icon_weapon",  icon_weapon_);
    context.Set<SDL_Texture*>("overlay.tex.crosshair",    crosshair_);

    // Map-select UI
    context.Set<SDL_Texture*>("overlay.tex.btn_back",     btn_back_);
    context.Set<SDL_Texture*>("overlay.tex.btn_fight",    btn_fight_);
    context.Set<SDL_Texture*>("overlay.tex.btn_skirmish", btn_skirmish_);
    context.Set<SDL_Texture*>("overlay.tex.arrow_l",      arrow_l_);
    context.Set<SDL_Texture*>("overlay.tex.arrow_r",      arrow_r_);

    context.Set<std::shared_ptr<ArenaMap>>      ("overlay.arena_data",      arena_data_);
    context.Set<std::shared_ptr<LevelshotCache>>("overlay.levelshot_cache", levelshot_cache_);
}
} // namespace sdl3cpp::services::impl
