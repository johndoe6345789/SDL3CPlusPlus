#include "services/interfaces/workflow/rendering/workflow_overlay_sw_begin_step.hpp"
#include "services/interfaces/workflow/quake3/q3_overlay_scale.hpp"

#include <nlohmann/json.hpp>
#include <SDL3/SDL.h>

#include <string>

namespace sdl3cpp::services::impl {

WorkflowOverlaySwBeginStep::WorkflowOverlaySwBeginStep(
    std::shared_ptr<ILogger> l)
    : logger_(std::move(l)) {}

WorkflowOverlaySwBeginStep::~WorkflowOverlaySwBeginStep() {
    if (renderer_) {
        DestroyOverlaySwBeginTextures(textures_);
        SDL_DestroyRenderer(renderer_);
    }
    if (surface_) SDL_DestroySurface(surface_);
}

std::string WorkflowOverlaySwBeginStep::GetPluginId() const {
    return "overlay.sw.begin";
}

void WorkflowOverlaySwBeginStep::Execute(const WorkflowStepDefinition& /*step*/,
                                         WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    if (!ready_) {
        // Rasterise at the display's resolution rather than at 640x480
        // and stretching, which resamples every glyph. Draw calls keep
        // working in 640x480: SDL's logical presentation scales the
        // geometry, the way ioq3's SCR_AdjustFrom640 does, and
        // letterboxes so a non-4:3 window cannot skew the menu.
        const auto size = q3::ChooseOverlaySize(
            static_cast<int>(context.Get<uint32_t>("frame_width", 1280u)),
            static_cast<int>(context.Get<uint32_t>("frame_height", 960u)));
        surface_ =
            SDL_CreateSurface(size.width, size.height, SDL_PIXELFORMAT_RGBA32);
        renderer_ = surface_ ? SDL_CreateSoftwareRenderer(surface_) : nullptr;
        if (renderer_) {
            SDL_SetRenderLogicalPresentation(
                renderer_, q3::kVirtualWidth, q3::kVirtualHeight,
                SDL_LOGICAL_PRESENTATION_LETTERBOX);
        }
        ready_ = surface_ && renderer_;
        if (ready_ && logger_) {
            logger_->Info("overlay.sw.begin: rasterising at " +
                          std::to_string(size.width) + "x" +
                          std::to_string(size.height) + " (" +
                          std::to_string(size.scale) + "x virtual)");
        }
    }
    if (!ready_) return;

    const auto bspCfg =
        context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    const std::string pk3 = bspCfg.value("pk3_path", std::string(""));
    if (!tex_loaded_ && !pk3.empty()) {
        LoadOverlaySwBeginTextures(renderer_, pk3, textures_);
        tex_loaded_ = true;
        if (logger_) {
            logger_->Info("overlay.sw.begin: textures loaded from " + pk3);
        }
    }

    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 0);
    SDL_RenderClear(renderer_);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    // Expose surface/renderer so drawing steps can use them
    context.Set<SDL_Renderer*>("overlay.renderer", renderer_);
    context.Set<SDL_Surface*>("overlay.surface", surface_);
    context.Set<bool>("overlay.ready", ready_);

    PublishOverlaySwBeginTextures(textures_, context);
}
}  // namespace sdl3cpp::services::impl
