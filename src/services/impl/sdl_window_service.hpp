#pragma once

#include "../interfaces/i_window_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../../di/lifecycle.hpp"
#include "../../events/event_bus.hpp"
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
    SdlWindowService(std::shared_ptr<ILogger> logger, std::shared_ptr<events::EventBus> eventBus);

    ~SdlWindowService() override;

    // IInitializable
    void Initialize() override;

    // IShutdownable
    void Shutdown() noexcept override;

    // IWindowService interface
    void CreateWindow(const WindowConfig& config) override;
    void DestroyWindow() override;
    SDL_Window* GetNativeHandle() const override { return window_; }
    std::pair<uint32_t, uint32_t> GetSize() const override;
    bool ShouldClose() const override { return shouldClose_; }
    void PollEvents() override;
    void SetTitle(const std::string& title) override;
    bool IsMinimized() const override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<events::EventBus> eventBus_;
    SDL_Window* window_ = nullptr;
    bool shouldClose_ = false;
    bool initialized_ = false;

    // Helper methods
    void PublishEvent(const SDL_Event& sdlEvent);
    double GetCurrentTime() const;
};

}  // namespace sdl3cpp::services::impl
