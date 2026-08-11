#include "services/interfaces/workflow/workflow_sdl_init_step.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include <SDL3/SDL.h>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowSdlInitStep::WorkflowSdlInitStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowSdlInitStep", "Constructor", "Entry");
    }
}

std::string WorkflowSdlInitStep::GetPluginId() const {
    return "sdl.init";
}

void WorkflowSdlInitStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowSdlInitStep", "Execute", "Entry");
    }

    try {
        // Initialize SDL3 with VIDEO subsystem (required for window + rendering)
        // Add AUDIO and INPUT if workflows need them
        const uint32_t flags = SDL_INIT_VIDEO;

        if (!SDL_Init(flags)) {
            if (logger_) {
                logger_->Error("WorkflowSdlInitStep::Execute: SDL_Init failed");
            }
            context.Set("sdl_initialized", false);
            return;
        }

        if (logger_) {
            logger_->Info("WorkflowSdlInitStep: SDL3 initialized successfully");
        }

        context.Set("sdl_initialized", true);

    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowSdlInitStep::Execute: " + std::string(e.what()));
        }
        context.Set("sdl_initialized", false);
    }
}

}  // namespace sdl3cpp::services::impl
