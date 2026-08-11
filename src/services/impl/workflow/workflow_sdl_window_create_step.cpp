#include "services/interfaces/workflow/workflow_sdl_window_create_step.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include <SDL3/SDL.h>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowSdlWindowCreateStep::WorkflowSdlWindowCreateStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowSdlWindowCreateStep", "Constructor", "Entry");
    }
}

std::string WorkflowSdlWindowCreateStep::GetPluginId() const {
    return "sdl.window.create";
}

void WorkflowSdlWindowCreateStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowSdlWindowCreateStep", "Execute", "Entry");
    }

    try {
        // Get parameters from context
        int width = context.GetInt("window_width", 1024);
        int height = context.GetInt("window_height", 768);
        std::string title = context.GetString("window_title", "SDL3 App");

        // Create window with hidden flag initially, then show it
        SDL_Window* window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_HIDDEN);
        if (!window) {
            // Try without hidden flag as fallback
            if (logger_) {
                logger_->Warn("WorkflowSdlWindowCreateStep: SDL_CreateWindow with HIDDEN failed, trying default");
            }
            window = SDL_CreateWindow(title.c_str(), width, height, 0);
        }
        if (window) {
            // Make window visible for rendering
            SDL_ShowWindow(window);
        }
        if (!window) {
            if (logger_) {
                logger_->Error("WorkflowSdlWindowCreateStep::Execute: SDL_CreateWindow failed");
            }
            context.Set("window_created", false);
            return;
        }

        // Store pointer in context as SDL_Window* for GPU steps
        context.Set<SDL_Window*>("sdl_window", window);
        context.Set("window_created", true);

        // Focus window and capture mouse for FPS controls
        SDL_RaiseWindow(window);
        SDL_SetWindowRelativeMouseMode(window, true);

        if (logger_) {
            logger_->Info("WorkflowSdlWindowCreateStep: Window created (" + std::to_string(width) + "x" +
                         std::to_string(height) + ") - " + title);
        }

    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowSdlWindowCreateStep::Execute: " + std::string(e.what()));
        }
        context.Set("window_created", false);
    }
}

}  // namespace sdl3cpp::services::impl
