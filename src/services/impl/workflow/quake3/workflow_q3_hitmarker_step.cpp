#include "services/interfaces/workflow/quake3/workflow_q3_hitmarker_step.hpp"
#include "services/interfaces/workflow/quake3/q3_overlay_utils.hpp"
#include <SDL3/SDL_render.h>
#include <cstdint>
namespace sdl3cpp::services::impl {
using namespace q3overlay;
WorkflowQ3HitmarkerStep::WorkflowQ3HitmarkerStep(std::shared_ptr<ILogger> l) : logger_(std::move(l)) {}
std::string WorkflowQ3HitmarkerStep::GetPluginId() const { return "q3.hitmarker"; }
void WorkflowQ3HitmarkerStep::Execute(const WorkflowStepDefinition&, WorkflowContext& context) {
    if (!context.GetBool("overlay.ready", false)) return;
    if (context.GetBool("q3.menu_open", false)) return;
    auto* r = context.Get<SDL_Renderer*>("overlay.renderer", nullptr);
    if (!r) return;
    const auto frame = (uint32_t)context.GetDouble("loop.iteration", 0.0);
    if (frame < context.Get<uint32_t>("q3.hit_marker_until_frame", 0u)) {
        SDL_SetRenderDrawColor(r, 255, 80, 55, 255);
        SDL_RenderLine(r, 308,172,320,160); SDL_RenderLine(r,332,172,320,160);
        SDL_RenderLine(r, 308,188,320,200); SDL_RenderLine(r,332,188,320,200);
    }
    if (frame < context.Get<uint32_t>("q3.weapon_flash_until_frame", 0u)) {
        SDL_SetRenderDrawColor(r, 255, 200, 60, 180);
        SDL_FRect flash{kW - 40.f, kH * 0.58f, 24.f, 40.f};
        SDL_RenderFillRect(r, &flash);
    }
}
} // namespace sdl3cpp::services::impl
