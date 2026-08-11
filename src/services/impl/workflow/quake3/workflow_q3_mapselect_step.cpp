#include "services/interfaces/workflow/quake3/workflow_q3_mapselect_step.hpp"
#include "services/interfaces/workflow/quake3/q3_overlay_utils.hpp"
#include <nlohmann/json.hpp>
#include <SDL3/SDL_render.h>
#include <algorithm>
#include <cctype>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {
using namespace q3overlay;
WorkflowQ3MapSelectStep::WorkflowQ3MapSelectStep(std::shared_ptr<ILogger> l) : logger_(std::move(l)) {}
std::string WorkflowQ3MapSelectStep::GetPluginId() const { return "q3.mapselect"; }
void WorkflowQ3MapSelectStep::Execute(const WorkflowStepDefinition&, WorkflowContext& context) {
    if (!context.GetBool("overlay.ready", false)) return;
    if (!context.GetBool("q3.menu_open", false)) return;
    if (context.Get<std::string>("q3.menu_screen","main") != "map_select") return;

    auto* r      = context.Get<SDL_Renderer*>("overlay.renderer", nullptr);
    auto* prop   = context.Get<SDL_Texture*>("overlay.tex.prop", nullptr);
    auto* bchars = context.Get<SDL_Texture*>("overlay.tex.bigchars", nullptr);
    if (!r) return;

    auto arenas = context.Get<std::shared_ptr<ArenaMap>>("overlay.arena_data", nullptr);
    auto shots  = context.Get<std::shared_ptr<LevelshotCache>>("overlay.levelshot_cache", nullptr);
    const auto bspCfg = context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    const std::string pk3 = bspCfg.value("pk3_path", std::string(""));

    // Black background
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_FRect bg{0,0,(float)kW,(float)kH}; SDL_RenderFillRect(r, &bg);

    constexpr float kCX = kW * 0.5f;
    DrawQ3Text(r, bchars, kCX-96.f, 12.f, "CHOOSE LEVEL", {200,50,20,255}, 1.0f);

    const auto maps  = context.Get<nlohmann::json>("q3.maps", nlohmann::json::array({"q3dm7"}));
    const int  sel   = context.Get<int>("q3.menu_selected_item", 0);
    const int  nMaps = (int)maps.size();
    const int  idx   = (nMaps>0) ? std::max(0,std::min(sel,nMaps-1)) : 0;
    std::string mapName = (nMaps>0) ? maps[(size_t)idx].get<std::string>() : "q3dm7";

    // Levelshot
    constexpr float kLW=162.f, kLH=162.f, kLX=(kW-kLW)*0.5f, kLY=58.f;
    SDL_Texture* shot = nullptr;
    if (shots && !mapName.empty() && !pk3.empty()) {
        std::string key = mapName;
        std::transform(key.begin(),key.end(),key.begin(),::toupper);
        auto it = shots->find(key);
        if (it != shots->end()) { shot = it->second; }
        else {
            shot = LoadTextureFromPk3(r, pk3, ("levelshots/"+key+".jpg").c_str());
            if (!shot) shot = LoadTextureFromPk3(r, pk3, ("levelshots/"+mapName+".jpg").c_str());
            (*shots)[key] = shot;
        }
    }
    if (shot) {
        SDL_FRect dst{kLX,kLY,kLW,kLH}; SDL_SetTextureAlphaMod(shot,255);
        SDL_SetTextureBlendMode(shot,SDL_BLENDMODE_NONE);
        SDL_RenderTexture(r,shot,nullptr,&dst);
    } else {
        SDL_SetRenderDrawColor(r,30,20,15,255); SDL_FRect fb{kLX,kLY,kLW,kLH}; SDL_RenderFillRect(r,&fb);
    }
    SDL_SetRenderDrawColor(r,180,50,20,255);
    SDL_FRect border{kLX-2.f,kLY-2.f,kLW+4.f,kLH+4.f}; SDL_RenderRect(r,&border);

    std::string mU=mapName; std::transform(mU.begin(),mU.end(),mU.begin(),::toupper);
    DrawPropText(r,prop,kCX,kLY+kLH+6.f,mU.c_str(),{255,200,50,255},0.7f,true);

    std::string mKey=mapName; std::transform(mKey.begin(),mKey.end(),mKey.begin(),::tolower);
    if (arenas) {
        auto aIt = arenas->find(mKey);
        if (aIt != arenas->end() && !aIt->second.longname.empty()) {
            std::string ln = mU+": "+aIt->second.longname;
            std::transform(ln.begin(),ln.end(),ln.begin(),::toupper);
            DrawPropText(r,prop,kCX,kLY+kLH+34.f,ln.c_str(),{200,130,40,255},0.75f,true);
            if (!aIt->second.bot.empty() && shots && !pk3.empty()) {
                const std::string botKey=aIt->second.bot;
                std::string bk=botKey; std::transform(bk.begin(),bk.end(),bk.begin(),::tolower);
                SDL_Texture* botIcon = nullptr;
                auto bIt = shots->find("bot_"+bk);
                if (bIt!=shots->end()) { botIcon=bIt->second; }
                else {
                    botIcon=LoadTextureFromPk3(r,pk3,("models/players/"+bk+"/icon_default.tga").c_str());
                    (*shots)["bot_"+bk]=botIcon;
                }
                if (botIcon) {
                    SDL_FRect dst{kCX-22.f,kLY+kLH+76.f,44.f,44.f};
                    SDL_SetTextureAlphaMod(botIcon,255); SDL_SetTextureBlendMode(botIcon,SDL_BLENDMODE_BLEND);
                    SDL_RenderTexture(r,botIcon,nullptr,&dst);
                }
                std::string bU=botKey; std::transform(bU.begin(),bU.end(),bU.begin(),::toupper);
                DrawPropText(r,prop,kCX,kLY+kLH+124.f,bU.c_str(),{180,130,50,220},0.65f,true);
            }
        }
    }

    // Nav arrows
    auto* al = context.Get<SDL_Texture*>("overlay.tex.arrow_l", nullptr);
    auto* ar = context.Get<SDL_Texture*>("overlay.tex.arrow_r", nullptr);
    constexpr float kAY = kLY+kLH*0.5f-16.f;
    if (al && idx>0) { SDL_FRect d{8.f,kAY,64.f,32.f}; SDL_RenderTexture(r,al,nullptr,&d); }
    if (ar && idx<nMaps-1) { SDL_FRect d{kW-72.f,kAY,64.f,32.f}; SDL_RenderTexture(r,ar,nullptr,&d); }

    // Buttons
    auto* bb = context.Get<SDL_Texture*>("overlay.tex.btn_back",     nullptr);
    auto* bf = context.Get<SDL_Texture*>("overlay.tex.btn_fight",    nullptr);
    auto* bs = context.Get<SDL_Texture*>("overlay.tex.btn_skirmish", nullptr);
    constexpr float kBY=(float)kH-52.f, kBW=128.f, kBH=46.f;
    auto dbtn=[&](SDL_Texture* t,float x){
        if(!t) return; SDL_SetTextureAlphaMod(t,255); SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND);
        SDL_FRect d{x,kBY,kBW,kBH}; SDL_RenderTexture(r,t,nullptr,&d);
    };
    dbtn(bb, kW*0.12f-kBW*0.5f);
    dbtn(bs, kW*0.50f-kBW*0.5f);
    dbtn(bf, kW*0.88f-kBW*0.5f);
}
} // namespace sdl3cpp::services::impl
