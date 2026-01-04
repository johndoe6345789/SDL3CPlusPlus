#pragma once

#include <cstdint>
#include <string>
#include <utility>

// Forward declare SDL types to avoid including SDL headers
struct SDL_Window;

namespace sdl3cpp::services {

/**
 * @brief Window configuration structure.
 */
struct WindowConfig {
    uint32_t width;
    uint32_t height;
    std::string title;
    bool resizable;
};

/**
 * @brief Window management service interface.
 *
 * Handles SDL window creation, event polling, and window state management.
 * Decouples SDL-specific details from the rest of the application.
 */
class IWindowService {
public:
    virtual ~IWindowService() = default;

    /**
     * @brief Create and show the application window.
     *
     * @param config Window configuration (size, title, flags)
     * @throws std::runtime_error if window creation fails
     */
    virtual void CreateWindow(const WindowConfig& config) = 0;

    /**
     * @brief Destroy the window and release resources.
     */
    virtual void DestroyWindow() = 0;

    /**
     * @brief Get the native SDL window handle.
     *
     * Required for Vulkan surface creation and other low-level operations.
     *
     * @return Pointer to SDL_Window, or nullptr if no window exists
     */
    virtual SDL_Window* GetNativeHandle() const = 0;

    /**
     * @brief Get the current window size.
     *
     * @return Pair of (width, height) in pixels
     */
    virtual std::pair<uint32_t, uint32_t> GetSize() const = 0;

    /**
     * @brief Check if the window should close.
     *
     * Returns true when the user clicks the close button or
     * a quit event is received.
     *
     * @return true if the application should exit, false otherwise
     */
    virtual bool ShouldClose() const = 0;

    /**
     * @brief Poll SDL events and publish them to the event bus.
     *
     * This should be called once per frame to process user input
     * and window events. Events are converted to application events
     * and published via the event bus.
     */
    virtual void PollEvents() = 0;

    /**
     * @brief Set the window title.
     *
     * @param title New window title
     */
    virtual void SetTitle(const std::string& title) = 0;

    /**
     * @brief Check if the window is minimized.
     *
     * @return true if minimized, false otherwise
     */
    virtual bool IsMinimized() const = 0;
};

}  // namespace sdl3cpp::services
