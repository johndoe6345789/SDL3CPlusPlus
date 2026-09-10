#pragma once
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow/quake3/q3_overlay_utils.hpp"
#include "services/interfaces/workflow/rendering/overlay_sw_begin_resources.hpp"
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
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
    SDL_Surface*  surface_  = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    bool ready_       = false;
    bool tex_loaded_  = false;
    OverlaySwBeginTextures textures_;
};
} // namespace sdl3cpp::services::impl
