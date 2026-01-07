#include "services/impl/bgfx_gui_service.hpp"
#include "services/interfaces/gui_types.hpp"

#include "services/interfaces/i_config_service.hpp"
#include "services/interfaces/i_logger.hpp"

#include <bgfx/bgfx.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {
class TestLogger : public sdl3cpp::services::ILogger {
public:
    void SetLevel(sdl3cpp::services::LogLevel level) override {
        level_ = level;
    }

    sdl3cpp::services::LogLevel GetLevel() const override {
        return level_;
    }

    void SetOutputFile(const std::string& filename) override {
        (void)filename;
    }

    void SetMaxLinesPerFile(size_t maxLines) override {
        (void)maxLines;
    }

    void EnableConsoleOutput(bool enable) override {
        consoleEnabled_ = enable;
    }

    void Log(sdl3cpp::services::LogLevel level, const std::string& message) override {
        if (level < level_) {
            return;
        }
        entries_.emplace_back(level, message);
        if (consoleEnabled_) {
            std::cout << message << '\n';
        }
    }

    void Trace(const std::string& message) override {
        Log(sdl3cpp::services::LogLevel::TRACE, message);
    }

    void Trace(const std::string& className,
               const std::string& methodName,
               const std::string& args = "",
               const std::string& message = "") override {
        std::string formattedMessage = className + "::" + methodName;
        if (!args.empty()) {
            formattedMessage += "(" + args + ")";
        }
        if (!message.empty()) {
            formattedMessage += ": " + message;
        }
        Log(sdl3cpp::services::LogLevel::TRACE, formattedMessage);
    }

    void Debug(const std::string& message) override {
        Log(sdl3cpp::services::LogLevel::DEBUG, message);
    }

    void Info(const std::string& message) override {
        Log(sdl3cpp::services::LogLevel::INFO, message);
    }

    void Warn(const std::string& message) override {
        Log(sdl3cpp::services::LogLevel::WARN, message);
    }

    void Error(const std::string& message) override {
        Log(sdl3cpp::services::LogLevel::ERROR, message);
    }

    void TraceFunction(const std::string& funcName) override {
        Trace("Entering " + funcName);
    }

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

    bool HasErrorSubstring(const std::string& fragment) const {
        for (const auto& entry : entries_) {
            if (entry.first == sdl3cpp::services::LogLevel::ERROR &&
                entry.second.find(fragment) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    bool HasSubstring(const std::string& fragment) const {
        for (const auto& entry : entries_) {
            if (entry.second.find(fragment) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

private:
    sdl3cpp::services::LogLevel level_ = sdl3cpp::services::LogLevel::TRACE;
    bool consoleEnabled_ = false;
    std::vector<std::pair<sdl3cpp::services::LogLevel, std::string>> entries_;
};

class StubConfigService : public sdl3cpp::services::IConfigService {
public:
    uint32_t GetWindowWidth() const override { return 1; }
    uint32_t GetWindowHeight() const override { return 1; }
    std::filesystem::path GetScriptPath() const override { return {}; }
    bool IsLuaDebugEnabled() const override { return false; }
    std::string GetWindowTitle() const override { return ""; }
    const sdl3cpp::services::InputBindings& GetInputBindings() const override { return inputBindings_; }
    const sdl3cpp::services::MouseGrabConfig& GetMouseGrabConfig() const override { return mouseGrabConfig_; }
    const sdl3cpp::services::BgfxConfig& GetBgfxConfig() const override { return bgfxConfig_; }
    const sdl3cpp::services::MaterialXConfig& GetMaterialXConfig() const override { return materialXConfig_; }
    const std::vector<sdl3cpp::services::MaterialXMaterialConfig>& GetMaterialXMaterialConfigs() const override {
        return materialXMaterials_;
    }
    const sdl3cpp::services::GuiFontConfig& GetGuiFontConfig() const override { return guiFontConfig_; }
    const std::string& GetConfigJson() const override { return configJson_; }

    void DisableFreeType() {
        guiFontConfig_.useFreeType = false;
    }

    void EnableMaterialXGuiShader() {
        materialXConfig_.enabled = true;
        materialXConfig_.useConstantColor = true;
        materialXConfig_.shaderKey = "gui";
        materialXConfig_.libraryPath = ResolveMaterialXLibraryPath();
    }

private:
    static std::filesystem::path ResolveMaterialXLibraryPath() {
        auto repoRoot = std::filesystem::path(__FILE__).parent_path().parent_path();
        return repoRoot / "MaterialX" / "libraries";
    }

    sdl3cpp::services::InputBindings inputBindings_{};
    sdl3cpp::services::MouseGrabConfig mouseGrabConfig_{};
    sdl3cpp::services::BgfxConfig bgfxConfig_{};
    sdl3cpp::services::MaterialXConfig materialXConfig_{};
    std::vector<sdl3cpp::services::MaterialXMaterialConfig> materialXMaterials_{};
    sdl3cpp::services::GuiFontConfig guiFontConfig_{};
    std::string configJson_{};
};

void Assert(bool condition, const std::string& message, int& failures) {
    if (!condition) {
        std::cerr << "test failure: " << message << '\n';
        ++failures;
    }
}
} // namespace

int main() {
    int failures = 0;

    bgfx::Init init{};
    init.type = bgfx::RendererType::Noop;
    init.resolution.width = 1;
    init.resolution.height = 1;
    init.resolution.reset = BGFX_RESET_NONE;

    if (!bgfx::init(init)) {
        std::cerr << "test failure: bgfx init failed (Noop renderer)" << '\n';
        return 1;
    }

    try {
        auto logger = std::make_shared<TestLogger>();
        auto configService = std::make_shared<StubConfigService>();
        configService->DisableFreeType();
        configService->EnableMaterialXGuiShader();

        sdl3cpp::services::impl::BgfxGuiService service(configService, logger);
        service.PrepareFrame({}, 1, 1);

        Assert(service.IsProgramReady(), "GUI shader program should link", failures);
        Assert(service.IsWhiteTextureReady(), "white texture should be created", failures);
        Assert(logger->HasSubstring("Using MaterialX GUI shaders"),
               "expected MaterialX GUI shader path", failures);

        if (!service.IsProgramReady() &&
            !logger->HasErrorSubstring("bgfx::createProgram failed to link shaders")) {
            Assert(false, "missing bgfx::createProgram link failure log", failures);
        }

        service.Shutdown();
    } catch (const std::exception& ex) {
        std::cerr << "exception during bgfx gui service tests: " << ex.what() << '\n';
        bgfx::shutdown();
        return 1;
    }

    bgfx::shutdown();

    if (failures == 0) {
        std::cout << "bgfx_gui_service_tests: PASSED" << '\n';
    } else {
        std::cerr << "bgfx_gui_service_tests: FAILED (" << failures << " errors)" << '\n';
    }

    return failures;
}
