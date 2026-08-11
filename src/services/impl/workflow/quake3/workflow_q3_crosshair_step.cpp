#include "services/interfaces/workflow/quake3/workflow_q3_crosshair_step.hpp"
#include "services/interfaces/workflow/quake3/q3_overlay_utils.hpp"
#include <SDL3/SDL_render.h>
namespace sdl3cpp::services::impl {
using namespace q3overlay;
WorkflowQ3CrosshairStep::WorkflowQ3CrosshairStep(std::shared_ptr<ILogger> l) : logger_(std::move(l)) {}
std::string WorkflowQ3CrosshairStep::GetPluginId() const { return "q3.crosshair"; }
void WorkflowQ3CrosshairStep::Execute(const WorkflowStepDefinition&, WorkflowContext& context) {
    if (!context.GetBool("overlay.ready", false)) return;
    if (context.GetBool("q3.menu_open", false)) return;
    auto* r = context.Get<SDL_Renderer*>("overlay.renderer", nullptr);
    if (!r) return;
    auto* ch = context.Get<SDL_Texture*>("overlay.tex.crosshair", nullptr);
    constexpr float kSize = 24.f;
    const float x = (kW - kSize) * 0.5f, y = (kH - kSize) * 0.5f;
    if (ch) {
        SDL_SetTextureAlphaMod(ch, 200); SDL_SetTextureColorMod(ch,255,255,255);
        SDL_SetTextureBlendMode(ch, SDL_BLENDMODE_BLEND);
        SDL_FRect dst{x, y, kSize, kSize}; SDL_RenderTexture(r, ch, nullptr, &dst);
    } else {
        SDL_SetRenderDrawColor(r, 255, 255, 255, 200);
        SDL_FRect dot{(kW-4.f)*0.5f,(kH-4.f)*0.5f,4.f,4.f};
        SDL_RenderFillRect(r, &dot);
    }
}
} // namespace sdl3cpp::services::impl
