#include "sdl_window_service.hpp"
#include "../interfaces/i_logger.hpp"
#include <SDL3/SDL_vulkan.h>
#include <chrono>
#include <sstream>
#include <stdexcept>

namespace sdl3cpp::services::impl {

namespace {

std::string BuildSdlErrorMessage(const char* context, const std::shared_ptr<IPlatformService>& platformService) {
    std::ostringstream oss;
    oss << context;
    const char* sdlError = SDL_GetError();
    if (sdlError && *sdlError != '\0') {
        oss << ": " << sdlError;
    } else {
        oss << ": (SDL_GetError returned an empty string)";
    }

    if (platformService) {
        std::string platformError = platformService->GetPlatformError();
        if (!platformError.empty() && platformError != "No platform error") {
            oss << " [" << platformError << "]";
        }
    }

    return oss.str();
}

void ThrowSdlErrorIfFailed(bool success, const char* context, const std::shared_ptr<IPlatformService>& platformService) {
    if (!success) {
        throw std::runtime_error(BuildSdlErrorMessage(context, platformService));
    }
}

void ShowErrorDialog(const char* title, const std::string& message) {
    // Disabled for headless environments
    // SDL_ShowSimpleMessageBox(
    //     SDL_MESSAGEBOX_ERROR,
    //     title,
    //     message.c_str(),
    //     nullptr);
}

}  // namespace

SdlWindowService::SdlWindowService(std::shared_ptr<ILogger> logger,
                                   std::shared_ptr<IPlatformService> platformService,
                                   std::shared_ptr<events::IEventBus> eventBus)
    : logger_(std::move(logger)),
      platformService_(std::move(platformService)),
      eventBus_(std::move(eventBus)) {
}

SdlWindowService::~SdlWindowService() {
    if (window_) {
        DestroyWindow();
    }
}

void SdlWindowService::Initialize() {
    logger_->TraceFunction(__func__);

    if (initialized_) {
        throw std::runtime_error("SdlWindowService already initialized");
    }

    // Defer SDL initialization until window creation to avoid issues in headless environments
    initialized_ = true;

    // Publish application started event
    eventBus_->Publish(events::Event{
        events::EventType::ApplicationStarted,
        GetCurrentTime(),
        {}
    });
}

void SdlWindowService::Shutdown() noexcept {
    if (!initialized_) {
        return;
    }

    if (window_) {
        DestroyWindow();
    }

    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
    initialized_ = false;
}

void SdlWindowService::CreateWindow(const WindowConfig& config) {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        throw std::runtime_error("SdlWindowService not initialized");
    }

    if (window_) {
        throw std::runtime_error("Window already created");
    }

    // Initialize SDL here if not already initialized
    if (SDL_WasInit(0) == 0) {
        try {
            ThrowSdlErrorIfFailed(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO), "SDL_Init failed", platformService_);
        } catch (const std::exception& e) {
            ShowErrorDialog("SDL Initialization Failed",
                std::string("Failed to initialize SDL subsystems.\n\nError: ") + e.what());
            throw;
        }

        try {
            ThrowSdlErrorIfFailed(SDL_Vulkan_LoadLibrary(nullptr), "SDL_Vulkan_LoadLibrary failed", platformService_);
        } catch (const std::exception& e) {
            ShowErrorDialog("Vulkan Library Load Failed",
                std::string("Failed to load Vulkan library. Make sure Vulkan drivers are installed.\n\nError: ") + e.what());
            throw;
        }
    }

    uint32_t flags = SDL_WINDOW_VULKAN;
    if (config.resizable) {
        flags |= SDL_WINDOW_RESIZABLE;
    }

    window_ = SDL_CreateWindow(
        config.title.c_str(),
        config.width,
        config.height,
        flags
    );

    if (!window_) {
        std::string errorMsg = BuildSdlErrorMessage("SDL_CreateWindow failed", platformService_);
        ShowErrorDialog("Window Creation Failed",
            std::string("Failed to create application window.\n\nError: ") + errorMsg);
        throw std::runtime_error(errorMsg);
    }

    SDL_StartTextInput(window_);

    logger_->TraceVariable("window_", reinterpret_cast<void*>(window_));
    logger_->TraceVariable("width", static_cast<int>(config.width));
    logger_->TraceVariable("height", static_cast<int>(config.height));
}

void SdlWindowService::DestroyWindow() {
    if (window_) {
        SDL_StopTextInput(window_);
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
}

std::pair<uint32_t, uint32_t> SdlWindowService::GetSize() const {
    if (!window_) {
        return {0, 0};
    }

    int width, height;
    SDL_GetWindowSizeInPixels(window_, &width, &height);
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

bool SdlWindowService::IsMinimized() const {
    if (!window_) {
        return false;
    }

    uint32_t flags = SDL_GetWindowFlags(window_);
    return (flags & SDL_WINDOW_MINIMIZED) != 0;
}

void SdlWindowService::PollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Convert SDL event to application event and publish
        PublishEvent(event);

        // Check for quit event
        if (event.type == SDL_EVENT_QUIT) {
            shouldClose_ = true;
        }
    }
}

void SdlWindowService::SetTitle(const std::string& title) {
    if (window_) {
        SDL_SetWindowTitle(window_, title.c_str());
    }
}

void SdlWindowService::PublishEvent(const SDL_Event& sdlEvent) {
    double timestamp = GetCurrentTime();

    switch (sdlEvent.type) {
        case SDL_EVENT_QUIT:
            eventBus_->Publish(events::Event{
                events::EventType::WindowClosed,
                timestamp,
                {}
            });
            break;

        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            eventBus_->Publish(events::Event{
                events::EventType::WindowResized,
                timestamp,
                events::WindowResizedEvent{
                    static_cast<uint32_t>(sdlEvent.window.data1),
                    static_cast<uint32_t>(sdlEvent.window.data2)
                }
            });
            break;

        case SDL_EVENT_WINDOW_MINIMIZED:
            eventBus_->Publish(events::Event{
                events::EventType::WindowMinimized,
                timestamp,
                {}
            });
            break;

        case SDL_EVENT_WINDOW_MAXIMIZED:
            eventBus_->Publish(events::Event{
                events::EventType::WindowMaximized,
                timestamp,
                {}
            });
            break;

        case SDL_EVENT_WINDOW_RESTORED:
            eventBus_->Publish(events::Event{
                events::EventType::WindowRestored,
                timestamp,
                {}
            });
            break;

        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            eventBus_->Publish(events::Event{
                events::EventType::WindowFocusGained,
                timestamp,
                {}
            });
            break;

        case SDL_EVENT_WINDOW_FOCUS_LOST:
            eventBus_->Publish(events::Event{
                events::EventType::WindowFocusLost,
                timestamp,
                {}
            });
            break;

        case SDL_EVENT_KEY_DOWN:
            eventBus_->Publish(events::Event{
                events::EventType::KeyPressed,
                timestamp,
                events::KeyEvent{
                    sdlEvent.key.key,
                    sdlEvent.key.scancode,
                    sdlEvent.key.mod,
                    sdlEvent.key.repeat
                }
            });
            break;

        case SDL_EVENT_KEY_UP:
            eventBus_->Publish(events::Event{
                events::EventType::KeyReleased,
                timestamp,
                events::KeyEvent{
                    sdlEvent.key.key,
                    sdlEvent.key.scancode,
                    sdlEvent.key.mod,
                    false
                }
            });
            break;

        case SDL_EVENT_MOUSE_MOTION:
            eventBus_->Publish(events::Event{
                events::EventType::MouseMoved,
                timestamp,
                events::MouseMovedEvent{
                    sdlEvent.motion.x,
                    sdlEvent.motion.y,
                    sdlEvent.motion.xrel,
                    sdlEvent.motion.yrel
                }
            });
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            eventBus_->Publish(events::Event{
                events::EventType::MouseButtonPressed,
                timestamp,
                events::MouseButtonEvent{
                    sdlEvent.button.button,
                    sdlEvent.button.clicks,
                    sdlEvent.button.x,
                    sdlEvent.button.y
                }
            });
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            eventBus_->Publish(events::Event{
                events::EventType::MouseButtonReleased,
                timestamp,
                events::MouseButtonEvent{
                    sdlEvent.button.button,
                    sdlEvent.button.clicks,
                    sdlEvent.button.x,
                    sdlEvent.button.y
                }
            });
            break;

        case SDL_EVENT_MOUSE_WHEEL:
            eventBus_->Publish(events::Event{
                events::EventType::MouseWheel,
                timestamp,
                events::MouseWheelEvent{
                    sdlEvent.wheel.x,
                    sdlEvent.wheel.y,
                    sdlEvent.wheel.direction == SDL_MOUSEWHEEL_FLIPPED
                }
            });
            break;

        case SDL_EVENT_TEXT_INPUT:
            eventBus_->Publish(events::Event{
                events::EventType::TextInput,
                timestamp,
                events::TextInputEvent{
                    std::string(sdlEvent.text.text)
                }
            });
            break;

        default:
            // Ignore other events
            break;
    }
}

double SdlWindowService::GetCurrentTime() const {
    using namespace std::chrono;
    auto now = steady_clock::now();
    auto dur = now.time_since_epoch();
    return duration_cast<std::chrono::duration<double>>(dur).count();
}

}  // namespace sdl3cpp::services::impl
