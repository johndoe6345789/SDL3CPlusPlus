#include "services/interfaces/workflow/quake3/workflow_q3_menu_frame_step.hpp"
#include "services/interfaces/workflow/quake3/q3_overlay_utils.hpp"
#include <nlohmann/json.hpp>
#include <SDL3/SDL_render.h>
namespace sdl3cpp::services::impl {
using namespace q3overlay;
WorkflowQ3MenuFrameStep::WorkflowQ3MenuFrameStep(std::shared_ptr<ILogger> l) : logger_(std::move(l)) {}
std::string WorkflowQ3MenuFrameStep::GetPluginId() const { return "q3.menu.frame"; }
void WorkflowQ3MenuFrameStep::Execute(const WorkflowStepDefinition&, WorkflowContext& context) {
    if (!context.GetBool("overlay.ready", false)) return;
    if (!context.GetBool("q3.menu_open", false)) return;
    const std::string screen = context.Get<std::string>("q3.menu_screen", "main");
    if (screen == "map_select") return;  // handled by q3.mapselect

    auto* r    = context.Get<SDL_Renderer*>("overlay.renderer", nullptr);
    auto* prop = context.Get<SDL_Texture*>("overlay.tex.prop", nullptr);
    auto* glo  = context.Get<SDL_Texture*>("overlay.tex.prop_glo", nullptr);
    auto* fl   = context.Get<SDL_Texture*>("overlay.tex.frame_l", nullptr);
    auto* fr   = context.Get<SDL_Texture*>("overlay.tex.frame_r", nullptr);
    if (!r) return;

    // Black background
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_FRect bg{0,0,(float)kW,(float)kH}; SDL_RenderFillRect(r, &bg);

    // Ring emblem
    constexpr float kRY=120.f, kRH=185.f, kHalf=kW*0.5f;
    if (fl) { SDL_FRect d{0.f,kRY,kHalf,kRH}; SDL_SetTextureAlphaMod(fl,255); SDL_RenderTexture(r,fl,nullptr,&d); }
    if (fr) { SDL_FRect d{kHalf,kRY,kHalf,kRH}; SDL_SetTextureAlphaMod(fr,255); SDL_RenderTexture(r,fr,nullptr,&d); }

    // Title with glow
    constexpr float kCX=kW*0.5f, kTY=20.f, kTS=1.5f, kSS=1.1f;
    const float kSY = kTY + kPropHeight*kTS + 2.f;
    if (glo) {
        DrawPropText(r,glo,kCX+2.f,kTY+2.f,"QUAKE III",{255,80,0,120},kTS,true);
        DrawPropText(r,glo,kCX+2.f,kSY+2.f,"ARENA",    {255,80,0,120},kSS,true);
    }
    DrawPropText(r,prop,kCX,kTY,"QUAKE III",{255,200,50,255},kTS,true);
    DrawPropText(r,prop,kCX,kSY,"ARENA",    {220,100,10,255},kSS,true);

    // Menu items
    const auto items  = context.Get<nlohmann::json>("q3.menu_items", nlohmann::json::array());
    const int  sel    = context.Get<int>("q3.menu_selected_item", 0);
    const int  nItems = (int)items.size();
    constexpr float kIS=1.0f, kIStep=kPropHeight*kIS+8.f, kITop=185.f;
    for (int i=0; i<nItems; ++i) {
        const std::string lbl = items[i].value("label","");
        const float iy = kITop + i*kIStep;
        if (i==sel) {
            if (glo) { DrawPropText(r,glo,kCX-1.f,iy,lbl.c_str(),{255,180,0,100},kIS,true);
                       DrawPropText(r,glo,kCX+1.f,iy,lbl.c_str(),{255,180,0,100},kIS,true); }
            DrawPropText(r,prop,kCX,iy,lbl.c_str(),{255,220,80,255},kIS,true);
        } else {
            DrawPropText(r,prop,kCX,iy,lbl.c_str(),{200,55,35,210},kIS,true);
        }
    }
    DrawPropText(r,prop,kCX,(float)(kH-18),
                 "Quake III Arena(c) 1999-2000, Id Software, Inc.",{180,50,30,200},0.42f,true);
}
} // namespace sdl3cpp::services::impl
