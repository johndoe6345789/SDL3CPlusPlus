#include "sdl_window_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include <algorithm>
#include <cctype>
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

std::string NormalizeMouseBindingName(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool TryParseMouseButton(const std::string& name, uint8_t& buttonOut) {
    if (name.empty()) {
        return false;
    }
    std::string normalized = NormalizeMouseBindingName(name);
    if (normalized == "left" || normalized == "button1") {
        buttonOut = SDL_BUTTON_LEFT;
        return true;
    }
    if (normalized == "right" || normalized == "button2") {
        buttonOut = SDL_BUTTON_RIGHT;
        return true;
    }
    if (normalized == "middle" || normalized == "button3") {
        buttonOut = SDL_BUTTON_MIDDLE;
        return true;
    }
    if (normalized == "x1" || normalized == "button4") {
        buttonOut = SDL_BUTTON_X1;
        return true;
    }
    if (normalized == "x2" || normalized == "button5") {
        buttonOut = SDL_BUTTON_X2;
        return true;
    }
    return false;
}

void ThrowSdlErrorIfFailed(bool success, const char* context, const std::shared_ptr<IPlatformService>& platformService) {
    if (!success) {
        throw std::runtime_error(BuildSdlErrorMessage(context, platformService));
    }
}

void ShowErrorDialog(const char* title, const std::string& message) {
    (void)title;
    (void)message;
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
    if (logger_) {
        logger_->Trace("SdlWindowService", "SdlWindowService",
                       "platformService=" + std::string(platformService_ ? "set" : "null") +
                       ", eventBus=" + std::string(eventBus_ ? "set" : "null"));
    }
}

SdlWindowService::~SdlWindowService() {
    if (logger_) {
        logger_->Trace("SdlWindowService", "~SdlWindowService");
    }
    if (window_) {
        DestroyWindow();
    }
}

void SdlWindowService::Initialize() {
    logger_->Trace("SdlWindowService", "Initialize");

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
    logger_->Trace("SdlWindowService", "Shutdown");
    if (!initialized_) {
        return;
    }

    if (window_) {
        DestroyWindow();
    }

    SDL_Quit();
    initialized_ = false;
}

void SdlWindowService::CreateWindow(const WindowConfig& config) {
    logger_->Trace("SdlWindowService", "CreateWindow",
                   "config.width=" + std::to_string(config.width) +
                   ", config.height=" + std::to_string(config.height) +
                   ", config.title=" + config.title +
                   ", config.resizable=" + std::string(config.resizable ? "true" : "false") +
                   ", mouseGrab.enabled=" + std::string(config.mouseGrab.enabled ? "true" : "false"));

    if (!initialized_) {
        throw std::runtime_error("SdlWindowService not initialized");
    }

    if (window_) {
        throw std::runtime_error("Window already created");
    }

    // Ensure SDL video is initialized even if another subsystem (like audio) was started first.
    const uint32_t initialized = SDL_WasInit(0);
    const bool videoInitialized = (initialized & SDL_INIT_VIDEO) != 0;
    logger_->TraceVariable("sdl.initializedFlags", static_cast<int>(initialized));
    logger_->TraceVariable("sdl.videoInitialized", videoInitialized);
    if (!videoInitialized) {
        try {
            if (initialized == 0) {
                logger_->Trace("SdlWindowService", "CreateWindow", "SDL_Init(SDL_INIT_VIDEO)");
                ThrowSdlErrorIfFailed(SDL_Init(SDL_INIT_VIDEO), "SDL_Init(SDL_INIT_VIDEO) failed", platformService_);
            } else {
                logger_->Trace("SdlWindowService", "CreateWindow", "SDL_InitSubSystem(SDL_INIT_VIDEO)");
                ThrowSdlErrorIfFailed(SDL_InitSubSystem(SDL_INIT_VIDEO),
                                      "SDL_InitSubSystem(SDL_INIT_VIDEO) failed",
                                      platformService_);
            }
        } catch (const std::exception& e) {
            ShowErrorDialog("SDL Initialization Failed",
                std::string("Failed to initialize SDL video subsystem.\n\nError: ") + e.what());
            throw;
        }
    }

    if (platformService_) {
        platformService_->LogSystemInfo();
    }

    uint32_t flags = 0;
    if (config.resizable) {
        flags |= SDL_WINDOW_RESIZABLE;
    }
    logger_->TraceVariable("sdl.windowFlags", static_cast<int>(flags));

    logger_->Trace("SdlWindowService", "CreateWindow", "SDL_CreateWindow");
    const int windowWidth = static_cast<int>(config.width);
    const int windowHeight = static_cast<int>(config.height);
    window_ = SDL_CreateWindow(
        config.title.c_str(),
        windowWidth,
        windowHeight,
        flags
    );

    if (!window_) {
        std::string errorMsg = BuildSdlErrorMessage("SDL_CreateWindow failed", platformService_);
        ShowErrorDialog("Window Creation Failed",
            std::string("Failed to create application window.\n\nError: ") + errorMsg);
        throw std::runtime_error(errorMsg);
    }

    logger_->Trace("SdlWindowService", "CreateWindow", "SDL_ShowWindow");
    SDL_ShowWindow(window_);

    SDL_StartTextInput(window_);

    mouseGrabConfig_ = config.mouseGrab;
    mouseGrabbed_ = false;
    ConfigureMouseGrabBindings();
    if (mouseGrabConfig_.enabled && mouseGrabConfig_.startGrabbed) {
        ApplyMouseGrab(true, false);
    }

    logger_->TraceVariable("window_", reinterpret_cast<void*>(window_));
    logger_->TraceVariable("width", static_cast<int>(config.width));
    logger_->TraceVariable("height", static_cast<int>(config.height));
}

void SdlWindowService::DestroyWindow() {
    logger_->Trace("SdlWindowService", "DestroyWindow",
                   "windowIsNull=" + std::string(window_ ? "false" : "true"));
    if (window_) {
        if (mouseGrabbed_) {
            ApplyMouseGrab(false, false);
        }
        SDL_StopTextInput(window_);
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
}

std::pair<uint32_t, uint32_t> SdlWindowService::GetSize() const {
    logger_->Trace("SdlWindowService", "GetSize",
                   "windowIsNull=" + std::string(window_ ? "false" : "true"));
    if (!window_) {
        return {0, 0};
    }

    int width, height;
    SDL_GetWindowSizeInPixels(window_, &width, &height);
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

bool SdlWindowService::IsMinimized() const {
    logger_->Trace("SdlWindowService", "IsMinimized",
                   "windowIsNull=" + std::string(window_ ? "false" : "true"));
    if (!window_) {
        return false;
    }

    const SDL_WindowFlags flags = SDL_GetWindowFlags(window_);
    return (flags & SDL_WINDOW_MINIMIZED) != 0;
}

void SdlWindowService::SetMouseGrabbed(bool grabbed) {
    if (logger_) {
        logger_->Trace("SdlWindowService", "SetMouseGrabbed",
                       "grabbed=" + std::string(grabbed ? "true" : "false"));
    }
    if (!window_) {
        return;
    }
    if (!mouseGrabConfig_.enabled) {
        mouseGrabConfig_.enabled = true;
    }
    ApplyMouseGrab(grabbed, true);
}

bool SdlWindowService::IsMouseGrabbed() const {
    if (logger_) {
        logger_->Trace("SdlWindowService", "IsMouseGrabbed");
    }
    return mouseGrabbed_;
}

void SdlWindowService::SetRelativeMouseMode(bool enabled) {
    if (logger_) {
        logger_->Trace("SdlWindowService", "SetRelativeMouseMode",
                       "enabled=" + std::string(enabled ? "true" : "false"));
    }
    mouseGrabConfig_.relativeMode = enabled;
    if (!window_) {
        return;
    }
    if (!SDL_SetWindowRelativeMouseMode(window_, enabled) && logger_) {
        logger_->Error("SdlWindowService: " +
                       BuildSdlErrorMessage("SDL_SetWindowRelativeMouseMode failed", platformService_));
    }
    if (mouseGrabbed_ && mouseGrabConfig_.enabled) {
        ApplyMouseGrab(mouseGrabbed_, true);
    }
}

bool SdlWindowService::IsRelativeMouseMode() const {
    if (logger_) {
        logger_->Trace("SdlWindowService", "IsRelativeMouseMode");
    }
    return mouseGrabConfig_.relativeMode;
}

void SdlWindowService::SetCursorVisible(bool visible) {
    if (logger_) {
        logger_->Trace("SdlWindowService", "SetCursorVisible",
                       "visible=" + std::string(visible ? "true" : "false"));
    }
    if (!window_) {
        return;
    }
    mouseGrabConfig_.hideCursor = !visible;
    bool cursorResult = visible ? SDL_ShowCursor() : SDL_HideCursor();
    if (!cursorResult && logger_) {
        logger_->Error("SdlWindowService: " +
                       BuildSdlErrorMessage(visible ? "SDL_ShowCursor failed" : "SDL_HideCursor failed",
                                            platformService_));
    } else {
        cursorVisible_ = visible;
    }
    if (mouseGrabbed_ && mouseGrabConfig_.enabled) {
        ApplyMouseGrab(mouseGrabbed_, true);
    }
}

bool SdlWindowService::IsCursorVisible() const {
    if (logger_) {
        logger_->Trace("SdlWindowService", "IsCursorVisible");
    }
    return cursorVisible_;
}

void SdlWindowService::PollEvents() {
    logger_->Trace("SdlWindowService", "PollEvents");
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Check if this event should trigger mouse grab
        bool isGrabTrigger = false;
        if (window_ && mouseGrabConfig_.enabled && mouseGrabConfig_.grabOnClick &&
            event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
            event.button.button == grabMouseButton_) {
            isGrabTrigger = true;
            if (logger_) {
                logger_->Info("SdlWindowService: Mouse grab triggered by click (button=" +
                             std::to_string(event.button.button) + ")");
            }
            ApplyMouseGrab(true, false);
        }

        // Handle release separately (doesn't suppress event)
        if (window_ && mouseGrabConfig_.enabled && mouseGrabConfig_.releaseOnEscape &&
            event.type == SDL_EVENT_KEY_DOWN &&
            event.key.key == releaseKey_ &&
            !event.key.repeat) {
            if (logger_) {
                logger_->Info("SdlWindowService: Mouse grab released by escape key");
            }
            ApplyMouseGrab(false, false);
        }

        // Only publish event if it's not the grab-triggering click
        if (!isGrabTrigger) {
            PublishEvent(event);
        }

        // Check for quit event
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            logger_->Trace("SdlWindowService", "PollEvents", "closeRequested=true");
            shouldClose_ = true;
        }
    }
}

void SdlWindowService::SetTitle(const std::string& title) {
    logger_->Trace("SdlWindowService", "SetTitle", "title=" + title);
    if (window_) {
        SDL_SetWindowTitle(window_, title.c_str());
    }
}

void SdlWindowService::ConfigureMouseGrabBindings() {
    grabMouseButton_ = SDL_BUTTON_LEFT;
    releaseKey_ = SDLK_ESCAPE;

    if (!mouseGrabConfig_.grabMouseButton.empty()) {
        uint8_t parsedButton = grabMouseButton_;
        if (TryParseMouseButton(mouseGrabConfig_.grabMouseButton, parsedButton)) {
            grabMouseButton_ = parsedButton;
        } else if (logger_) {
            logger_->Error("SdlWindowService: unknown mouse grab button '" +
                           mouseGrabConfig_.grabMouseButton + "'");
        }
    }

    if (!mouseGrabConfig_.releaseKey.empty()) {
        SDL_Keycode parsedKey = SDL_GetKeyFromName(mouseGrabConfig_.releaseKey.c_str());
        if (parsedKey != SDLK_UNKNOWN) {
            releaseKey_ = parsedKey;
        } else if (logger_) {
            logger_->Error("SdlWindowService: unknown mouse release key '" +
                           mouseGrabConfig_.releaseKey + "'");
        }
    }

    if (logger_) {
        logger_->Info("SdlWindowService: Mouse grab config: enabled=" +
                     std::string(mouseGrabConfig_.enabled ? "true" : "false") +
                     ", grabOnClick=" + std::string(mouseGrabConfig_.grabOnClick ? "true" : "false") +
                     ", grabMouseButton=" + std::to_string(static_cast<int>(grabMouseButton_)) +
                     ", releaseKey=" + std::to_string(static_cast<int>(releaseKey_)));
    }
}

void SdlWindowService::HandleMouseGrabEvent(const SDL_Event& sdlEvent) {
    if (!window_ || !mouseGrabConfig_.enabled) {
        return;
    }

    if (mouseGrabConfig_.grabOnClick &&
        sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
        sdlEvent.button.button == grabMouseButton_) {
        ApplyMouseGrab(true, false);
        return;
    }

    if (mouseGrabConfig_.releaseOnEscape &&
        sdlEvent.type == SDL_EVENT_KEY_DOWN &&
        sdlEvent.key.key == releaseKey_ &&
        !sdlEvent.key.repeat) {
        ApplyMouseGrab(false, false);
    }
}

void SdlWindowService::ApplyMouseGrab(bool grabbed, bool force) {
    if (!window_ || !mouseGrabConfig_.enabled) {
        return;
    }
    if (!force && mouseGrabbed_ == grabbed) {
        return;
    }

    if (logger_) {
        logger_->Trace("SdlWindowService", "ApplyMouseGrab",
                       "grabbed=" + std::string(grabbed ? "true" : "false") +
                       ", relativeMode=" + std::string(mouseGrabConfig_.relativeMode ? "true" : "false") +
                       ", hideCursor=" + std::string(mouseGrabConfig_.hideCursor ? "true" : "false"));
    }

    bool success = true;
    if (mouseGrabConfig_.relativeMode) {
        if (!SDL_SetWindowRelativeMouseMode(window_, grabbed)) {
            success = false;
            if (logger_) {
                logger_->Error("SdlWindowService: " +
                               BuildSdlErrorMessage("SDL_SetWindowRelativeMouseMode failed", platformService_));
            }
        }
    }

    if (!SDL_SetWindowMouseGrab(window_, grabbed)) {
        success = false;
        if (logger_) {
            logger_->Error("SdlWindowService: " +
                           BuildSdlErrorMessage("SDL_SetWindowMouseGrab failed", platformService_));
        }
    }

    if (mouseGrabConfig_.hideCursor) {
        bool cursorResult = grabbed ? SDL_HideCursor() : SDL_ShowCursor();
        if (!cursorResult && logger_) {
            logger_->Error("SdlWindowService: " +
                           BuildSdlErrorMessage(grabbed ? "SDL_HideCursor failed" : "SDL_ShowCursor failed",
                                                platformService_));
        } else {
            cursorVisible_ = !grabbed;
        }
    } else {
        bool cursorResult = SDL_ShowCursor();
        if (!cursorResult && logger_) {
            logger_->Error("SdlWindowService: " +
                           BuildSdlErrorMessage("SDL_ShowCursor failed", platformService_));
        } else {
            cursorVisible_ = true;
        }
    }

    if (success) {
        mouseGrabbed_ = grabbed;
        if (eventBus_) {
            eventBus_->Publish(events::Event{
                events::EventType::MouseGrabChanged,
                GetCurrentTime(),
                events::MouseGrabEvent{grabbed}
            });
        }
        if (logger_) {
            logger_->Trace("SdlWindowService", "ApplyMouseGrab",
                           "mouseGrabChanged=true, grabbed=" + std::string(grabbed ? "true" : "false"));
        }
    } else if (logger_) {
        logger_->Trace("SdlWindowService", "ApplyMouseGrab", "grabChangeFailed=true");
    }
}

void SdlWindowService::PublishEvent(const SDL_Event& sdlEvent) {
    logger_->Trace("SdlWindowService", "PublishEvent",
                   "eventType=" + std::to_string(static_cast<int>(sdlEvent.type)));
    double timestamp = GetCurrentTime();

    switch (sdlEvent.type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
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
    logger_->Trace("SdlWindowService", "GetCurrentTime");
    using namespace std::chrono;
    auto now = steady_clock::now();
    auto dur = now.time_since_epoch();
    return duration_cast<std::chrono::duration<double>>(dur).count();
}

}  // namespace sdl3cpp::services::impl
