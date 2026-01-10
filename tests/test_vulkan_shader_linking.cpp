#include "services/impl/graphics/bgfx_graphics_backend.hpp"
#include "services/impl/gui/bgfx_gui_service.hpp"
#include "services/impl/config/json_config_service.hpp"
#include "services/impl/platform/platform_service.hpp"
#include "services/impl/platform/sdl_window_service.hpp"
#include "services/impl/shader/pipeline_compiler_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/gui_types.hpp"
#include "events/event_bus.hpp"

#include <bgfx/bgfx.h>
#include <SDL3/SDL.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

namespace {

class TestLogger : public sdl3cpp::services::ILogger {
public:
    void SetLevel(sdl3cpp::services::LogLevel level) override { level_ = level; }
    sdl3cpp::services::LogLevel GetLevel() const override { return level_; }
    void SetOutputFile(const std::string& filename) override { (void)filename; }
    void SetMaxLinesPerFile(size_t maxLines) override { (void)maxLines; }
    void EnableConsoleOutput(bool enable) override { consoleEnabled_ = enable; }

    void Log(sdl3cpp::services::LogLevel level, const std::string& message) override {
        if (level < level_) {
            return;
        }
        if (consoleEnabled_) {
            std::cout << message << '\n';
        }
        if (level == sdl3cpp::services::LogLevel::ERROR) {
            errorMessages_.push_back(message);
        }
    }

    void Trace(const std::string& message) override { Log(sdl3cpp::services::LogLevel::TRACE, message); }
    void Trace(const std::string& className, const std::string& methodName,
               const std::string& args = "", const std::string& message = "") override {
        std::string formattedMessage = className + "::" + methodName;
        if (!args.empty()) {
            formattedMessage += "(" + args + ")";
        }
        if (!message.empty()) {
            formattedMessage += ": " + message;
        }
        Log(sdl3cpp::services::LogLevel::TRACE, formattedMessage);
    }
    void Debug(const std::string& message) override { Log(sdl3cpp::services::LogLevel::DEBUG, message); }
    void Info(const std::string& message) override { Log(sdl3cpp::services::LogLevel::INFO, message); }
    void Warn(const std::string& message) override { Log(sdl3cpp::services::LogLevel::WARN, message); }
    void Error(const std::string& message) override { Log(sdl3cpp::services::LogLevel::ERROR, message); }
    void TraceFunction(const std::string& funcName) override { Trace("Entering " + funcName); }
    void TraceVariable(const std::string& name, const std::string& value) override {
        Trace(name + " = " + value);
    }
    void TraceVariable(const std::string& name, int value) override {
        TraceVariable(name, std::to_string(value));
    }
    void TraceVariable(const std::string& name, size_t value) override {
        TraceVariable(name, std::to_string(value));
    }
    void TraceVariable(const std::string& name, bool value) override {
        TraceVariable(name, std::string(value ? "true" : "false"));
    }
    void TraceVariable(const std::string& name, float value) override {
        TraceVariable(name, std::to_string(value));
    }
    void TraceVariable(const std::string& name, double value) override {
        TraceVariable(name, std::to_string(value));
    }

    bool HasError(const std::string& substring) const {
        for (const auto& msg : errorMessages_) {
            if (msg.find(substring) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    const std::vector<std::string>& GetErrors() const { return errorMessages_; }

private:
    sdl3cpp::services::LogLevel level_ = sdl3cpp::services::LogLevel::TRACE;
    bool consoleEnabled_ = false;
    std::vector<std::string> errorMessages_;
};

void Assert(bool condition, const std::string& message, int& failures) {
    if (!condition) {
        std::cerr << "TEST FAILED: " << message << '\n';
        ++failures;
    }
}

}  // namespace

// TDD Test: Verify GUI shaders compile and link successfully
// This test validates that walls/ceiling/floor render correctly by ensuring
// the GUI shader program links without errors.
int main() {
    int failures = 0;

    auto logger = std::make_shared<TestLogger>();
    logger->EnableConsoleOutput(true);
    logger->SetLevel(sdl3cpp::services::LogLevel::TRACE);

    // Setup config with seed_runtime.json (production config that was failing)
    std::filesystem::path configPath = "config/seed_runtime.json";
    if (!std::filesystem::exists(configPath)) {
        std::cerr << "SKIPPED: Config file not found: " << configPath << '\n';
        return 0;
    }

    std::cout << "Loading config from: " << configPath << '\n';
    auto configService = std::make_shared<sdl3cpp::services::impl::JsonConfigService>(logger, configPath, false);
    auto platformService = std::make_shared<sdl3cpp::services::impl::PlatformService>(logger);
    auto pipelineCompilerService = std::make_shared<sdl3cpp::services::impl::PipelineCompilerService>(logger);
    
    // Setup window service
    auto eventBus = std::make_shared<sdl3cpp::events::EventBus>();
    auto windowService = std::make_shared<sdl3cpp::services::impl::SdlWindowService>(
        logger, platformService, eventBus);

    // Try to create window with available drivers
    const char* preferredDrivers[] = {"x11", "wayland", "offscreen", "dummy", nullptr};
    SDL_Window* window = nullptr;
    bool windowCreated = false;

    std::cout << "Attempting to create window...\n";
    for (const char* driver : preferredDrivers) {
        if (driver) {
            SDL_SetHint(SDL_HINT_VIDEO_DRIVER, driver);
        }
        
        try {
            windowService->Initialize();
            sdl3cpp::services::WindowConfig config{};
            config.width = configService->GetWindowWidth();
            config.height = configService->GetWindowHeight();
            config.title = configService->GetWindowTitle();
            config.resizable = false;
            windowService->CreateWindow(config);
            window = windowService->GetNativeHandle();
            windowCreated = true;
            std::cout << "Created window with driver: " << (driver ? driver : "default") << '\n';
            break;
        } catch (const std::exception& ex) {
            std::cerr << "Window creation failed with driver " << (driver ? driver : "default") << ": " << ex.what() << '\n';
            windowService->Shutdown();
            windowService = std::make_shared<sdl3cpp::services::impl::SdlWindowService>(
                logger, platformService, eventBus);
        }
    }

    if (!windowCreated) {
        std::cerr << "SKIPPED: Could not create SDL window\n";
        return 0;
    }

    // Initialize graphics backend (this will init bgfx with production renderer settings)
    sdl3cpp::services::impl::BgfxGraphicsBackend backend(configService, platformService, logger, pipelineCompilerService);
    
    std::cout << "Attempting to initialize bgfx...\n";
    try {
        sdl3cpp::services::GraphicsConfig graphicsConfig{};
        backend.Initialize(window, graphicsConfig);
        std::cout << "bgfx initialized successfully\n";
    } catch (const std::exception& ex) {
        std::cerr << "bgfx init exception: " << ex.what() << '\n';
        backend.Shutdown();
        windowService->Shutdown();
        return 0;
    }

    const bgfx::RendererType::Enum rendererType = bgfx::getRendererType();
    std::cout << "Testing with renderer: " << bgfx::getRendererName(rendererType) << '\n';

    if (rendererType == bgfx::RendererType::Noop) {
        std::cerr << "SKIPPED: Got Noop renderer\n";
        backend.Shutdown();
        windowService->Shutdown();
        return 0;
    }

    // Now test GUI service shader compilation and linking (same as production)
    sdl3cpp::services::impl::BgfxGuiService guiService(configService, logger, pipelineCompilerService);
    
    // This triggers shader compilation and program linking
    const uint32_t width = configService->GetWindowWidth();
    const uint32_t height = configService->GetWindowHeight();
    guiService.PrepareFrame({}, width, height);

    // CRITICAL TESTS: These are the exact errors from current_problem/README.md
    // If these errors occur, walls/ceiling/floor won't render!
    
    Assert(!logger->HasError("failed to link shaders"),
           "FAIL: Shader program linking failed - walls/ceiling/floor won't render!",
           failures);

    Assert(!logger->HasError("Failed to create GUI shader program"),
           "FAIL: GUI shader program creation failed - walls/ceiling/floor won't render!",
           failures);

    Assert(guiService.IsProgramReady(),
           "FAIL: GUI program not ready - walls/ceiling/floor won't render!",
           failures);

    // Print any errors that occurred
    const auto& errors = logger->GetErrors();
    if (!errors.empty()) {
        std::cerr << "\nErrors during shader compilation/linking:\n";
        for (const auto& error : errors) {
            std::cerr << "  - " << error << '\n';
        }
        std::cerr << "\nThese errors will cause missing geometry (walls/ceiling/floor) in the rendered scene.\n";
    }

    if (failures == 0) {
        std::cout << "\nSUCCESS: GUI shaders compiled and linked successfully with "
                  << bgfx::getRendererName(rendererType) << '\n';
        std::cout << "Walls, ceiling, and floor should render correctly in production.\n";
    }

    guiService.Shutdown();
    backend.Shutdown();
    windowService->Shutdown();

    if (failures == 0) {
        std::cout << "\nvulkan_shader_linking_test: PASSED\n";
    } else {
        std::cerr << "\nvulkan_shader_linking_test: FAILED (" << failures << " errors)\n";
        std::cerr << "Fix: Ensure shader uniforms are properly embedded in shader binaries.\n";
    }

    return failures;
}
