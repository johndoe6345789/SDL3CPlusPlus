#include "services/interfaces/workflow/quake3/workflow_q3_hud_step.hpp"
#include "services/interfaces/workflow/quake3/q3_hud_draw.hpp"

#include <string>

namespace sdl3cpp::services::impl {

WorkflowQ3HudStep::WorkflowQ3HudStep(std::shared_ptr<ILogger> l)
    : logger_(std::move(l)) {}

std::string WorkflowQ3HudStep::GetPluginId() const { return "q3.hud"; }

void WorkflowQ3HudStep::Execute(const WorkflowStepDefinition&,
                               WorkflowContext& context) {
    if (!context.GetBool("overlay.ready", false)) return;
    if (context.GetBool("q3.menu_open", false)) return;

    Q3HudAssets assets;
    assets.renderer = context.Get<SDL_Renderer*>("overlay.renderer", nullptr);
    if (!assets.renderer) return;

    for (int i = 0; i < 11; ++i) {
        assets.digits[i] = context.Get<SDL_Texture*>(
            "overlay.tex.num." + std::to_string(i), nullptr);
    }
    assets.iconArmor =
        context.Get<SDL_Texture*>("overlay.tex.icon_armor", nullptr);
    assets.iconWeapon =
        context.Get<SDL_Texture*>("overlay.tex.icon_weapon", nullptr);
    assets.iconFace =
        context.Get<SDL_Texture*>("overlay.tex.icon_face", nullptr);
    assets.headGpuTex =
        context.Get<SDL_GPUTexture*>("overlay.head_gpu_tex", nullptr);

    assets.health = context.Get<int>("q3.player_health", 100);
    assets.armor = context.Get<int>("q3.player_armor", 0);
    assets.ammo = context.Get<int>("q3.player_ammo", 50);

    const Q3HudFaceRect faceRect = DrawQ3Hud(assets);

    // q3.hud_head_render reads these to position its quad on the
    // swapchain.
    context.Set<float>("hud.face_rect_x", faceRect.x);
    context.Set<float>("hud.face_rect_y", faceRect.y);
    context.Set<float>("hud.face_rect_w", faceRect.w);
    context.Set<float>("hud.face_rect_h", faceRect.h);
}

}  // namespace sdl3cpp::services::impl
