#pragma once

#include "services/interfaces/i_window_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_platform_service.hpp"
#include "../../../di/lifecycle.hpp"
#include "../../../events/i_event_bus.hpp"
#include <memory>
#include <SDL3/SDL.h>

namespace sdl3cpp::services::impl {

/**
 * @brief SDL3-based window service implementation.
 *
 * Manages SDL window lifecycle, event polling, and publishes
 * window/input events to the event bus.
 */
class SdlWindowService : public IWindowService,
                         public di::IInitializable,
                         public di::IShutdownable {
public:
    /**
     * @brief Construct with event bus dependency.
     *
     * @param logger Logger service for logging
     * @param eventBus Event bus for publishing window/input events
     */
    SdlWindowService(std::shared_ptr<ILogger> logger,
                     std::shared_ptr<IPlatformService> platformService,
                     std::shared_ptr<events::IEventBus> eventBus);

    ~SdlWindowService() override;

    // IInitializable
    void Initialize() override;

    // IShutdownable
    void Shutdown() noexcept override;

    // IWindowService interface
    void CreateWindow(const WindowConfig& config) override;
    void DestroyWindow() override;
    SDL_Window* GetNativeHandle() const override {
        logger_->Trace("SdlWindowService", "GetNativeHandle");
        return window_;
    }
    std::pair<uint32_t, uint32_t> GetSize() const override;
    bool ShouldClose() const override {
        logger_->Trace("SdlWindowService", "ShouldClose");
        return shouldClose_;
    }
    void PollEvents() override;
    void SetTitle(const std::string& title) override;
    bool IsMinimized() const override;
    void SetMouseGrabbed(bool grabbed) override;
    bool IsMouseGrabbed() const override;
    void SetRelativeMouseMode(bool enabled) override;
    bool IsRelativeMouseMode() const override;
    void SetCursorVisible(bool visible) override;
    bool IsCursorVisible() const override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IPlatformService> platformService_;
    std::shared_ptr<events::IEventBus> eventBus_;
    SDL_Window* window_ = nullptr;
    bool shouldClose_ = false;
    bool initialized_ = false;
    MouseGrabConfig mouseGrabConfig_{};
    bool mouseGrabbed_ = false;
    bool cursorVisible_ = true;
    uint8_t grabMouseButton_ = SDL_BUTTON_LEFT;
    SDL_Keycode releaseKey_ = SDLK_ESCAPE;

    // Helper methods
    void PublishEvent(const SDL_Event& sdlEvent);
    double GetCurrentTime() const;
    void HandleMouseGrabEvent(const SDL_Event& sdlEvent);
    void ApplyMouseGrab(bool grabbed, bool force);
    void ConfigureMouseGrabBindings();
};

}  // namespace sdl3cpp::services::impl
