#pragma once
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow/quake3/q3_overlay_utils.hpp"
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {
class WorkflowOverlaySwBeginStep final : public IWorkflowStep {
public:
    explicit WorkflowOverlaySwBeginStep(std::shared_ptr<ILogger> logger);
    ~WorkflowOverlaySwBeginStep();
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    void TryLoadTextures(const std::string& pk3Path);
    std::shared_ptr<ILogger> logger_;
    SDL_Surface*  surface_  = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    bool ready_       = false;
    bool tex_loaded_  = false;
    SDL_Texture* bigchars_  = nullptr;
    SDL_Texture* prop_      = nullptr;
    SDL_Texture* prop_glo_  = nullptr;
    SDL_Texture* frame_l_   = nullptr;
    SDL_Texture* frame_r_   = nullptr;
    SDL_Texture* digits_[11]= {};
    SDL_Texture* icon_armor_   = nullptr;
    SDL_Texture* icon_health_  = nullptr;
    SDL_Texture* icon_face_    = nullptr;
    SDL_Texture* icon_weapon_  = nullptr;  // iconw_machinegun — weapon icon on HUD right side
    SDL_Texture* crosshair_    = nullptr;
    SDL_Texture* btn_back_     = nullptr;
    SDL_Texture* btn_fight_    = nullptr;
    SDL_Texture* btn_skirmish_ = nullptr;
    SDL_Texture* arrow_l_      = nullptr;
    SDL_Texture* arrow_r_      = nullptr;
    std::shared_ptr<q3overlay::ArenaMap>       arena_data_;
    std::shared_ptr<q3overlay::LevelshotCache> levelshot_cache_;
};
} // namespace sdl3cpp::services::impl
