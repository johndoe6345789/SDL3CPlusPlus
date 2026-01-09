#include "services/impl/bgfx_graphics_backend.hpp"
#include "services/impl/logger_service.hpp"
#include "services/impl/mesh_service.hpp"
#include "services/impl/pipeline_compiler_service.hpp"
#include "services/impl/physics_bridge_service.hpp"
#include "services/impl/platform_service.hpp"
#include "services/impl/script_engine_service.hpp"
#include "services/impl/scene_script_service.hpp"
#include "services/impl/shader_script_service.hpp"
#include "services/impl/shader_system_registry.hpp"
#include "services/impl/ecs_service.hpp"
#include "services/impl/scene_service.hpp"
#include "services/impl/sdl_window_service.hpp"
#include "services/interfaces/i_audio_command_service.hpp"
#include "services/interfaces/i_config_service.hpp"
#include "events/event_bus.hpp"

#include <array>
#include <bgfx/bgfx.h>
#include <cmath>
#include <limits>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <lua.hpp>
#include <memory>
#include <optional>
#include <SDL3/SDL.h>
#include <string>
#include <vector>

namespace {
constexpr std::array<float, 16> kIdentityMatrix = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};

bool ApproximatelyEqual(float a, float b, float eps = 1e-5f) {
    return std::fabs(a - b) <= eps;
}

bool ExpectIdentity(const std::array<float, 16>& actual, const std::string& label, int& failures) {
    for (size_t i = 0; i < actual.size(); ++i) {
        if (!ApproximatelyEqual(actual[i], kIdentityMatrix[i])) {
            std::cerr << label << " differs at index " << i << " (" << actual[i] << " vs "
                      << kIdentityMatrix[i] << ")\n";
            ++failures;
            return false;
        }
    }
    return true;
}

std::filesystem::path GetTestScriptPath() {
    auto testDir = std::filesystem::path(__FILE__).parent_path();
    return testDir / "scripts" / "unit_cube_logic.lua";
}

std::filesystem::path GetCubeScriptPath() {
    auto repoRoot = std::filesystem::path(__FILE__).parent_path().parent_path();
    return repoRoot / "scripts" / "cube_logic.lua";
}

std::filesystem::path GetSeedConfigPath() {
    auto repoRoot = std::filesystem::path(__FILE__).parent_path().parent_path();
    return repoRoot / "config" / "seed_runtime.json";
}

std::string DeterminePreferredRenderer() {
    if (const char* envRenderer = std::getenv("BGFX_RENDERER")) {
        return envRenderer;
    }
    if (const char* videoDriver = std::getenv("SDL_VIDEODRIVER")) {
        std::string driver(videoDriver);
        if (driver == "offscreen" || driver == "dummy") {
            return "opengl";
        }
    }
    return "opengl";
}

std::optional<std::string> ReadFileContents(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        return std::nullopt;
    }
    std::string contents((std::istreambuf_iterator<char>(input)),
                         std::istreambuf_iterator<char>());
    return contents;
}

class StubConfigService final : public sdl3cpp::services::IConfigService {
public:
    StubConfigService() {
        materialXConfig_.enabled = true;
        materialXConfig_.useConstantColor = true;
        materialXConfig_.shaderKey = "test";
        materialXConfig_.libraryPath = ResolveMaterialXLibraryPath();
        auto configJson = ReadFileContents(GetSeedConfigPath());
        if (configJson) {
            configJson_ = *configJson;
        } else {
            configJson_ = "{}";
        }
    }

    uint32_t GetWindowWidth() const override { return 1; }
    uint32_t GetWindowHeight() const override { return 1; }
    std::filesystem::path GetScriptPath() const override { return {}; }
    bool IsLuaDebugEnabled() const override { return false; }
    std::string GetWindowTitle() const override { return ""; }
    sdl3cpp::services::SceneSource GetSceneSource() const override {
        return sdl3cpp::services::SceneSource::Lua;
    }
    const sdl3cpp::services::InputBindings& GetInputBindings() const override { return inputBindings_; }
    const sdl3cpp::services::MouseGrabConfig& GetMouseGrabConfig() const override { return mouseGrabConfig_; }
    const sdl3cpp::services::BgfxConfig& GetBgfxConfig() const override { return bgfxConfig_; }
    const sdl3cpp::services::MaterialXConfig& GetMaterialXConfig() const override { return materialXConfig_; }
    const std::vector<sdl3cpp::services::MaterialXMaterialConfig>& GetMaterialXMaterialConfigs() const override {
        return materialXMaterials_;
    }
    const sdl3cpp::services::GuiFontConfig& GetGuiFontConfig() const override { return guiFontConfig_; }
    const sdl3cpp::services::RenderBudgetConfig& GetRenderBudgetConfig() const override { return budgets_; }
    const sdl3cpp::services::CrashRecoveryConfig& GetCrashRecoveryConfig() const override { return crashRecovery_; }
    const sdl3cpp::services::ValidationTourConfig& GetValidationTourConfig() const override {
        return validationTour_;
    }
    const std::string& GetConfigJson() const override { return configJson_; }

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
    sdl3cpp::services::RenderBudgetConfig budgets_{};
    sdl3cpp::services::CrashRecoveryConfig crashRecovery_{};
    sdl3cpp::services::ValidationTourConfig validationTour_{};
    std::string configJson_{};
};

class CubeDemoConfigService final : public sdl3cpp::services::IConfigService {
public:
    CubeDemoConfigService(std::filesystem::path scriptPath, std::string configJson, std::string renderer = "auto")
        : scriptPath_(std::move(scriptPath)),
          configJson_(std::move(configJson)) {
        materialXConfig_.enabled = true;
        materialXConfig_.useConstantColor = true;
        materialXConfig_.shaderKey = "test";
        materialXConfig_.libraryPath = ResolveMaterialXLibraryPath();
        renderer_ = std::move(renderer);
        bgfxConfig_.renderer = renderer_;
    }

    uint32_t GetWindowWidth() const override { return 1; }
    uint32_t GetWindowHeight() const override { return 1; }
    std::filesystem::path GetScriptPath() const override { return scriptPath_; }
    bool IsLuaDebugEnabled() const override { return false; }
    std::string GetWindowTitle() const override { return ""; }
    sdl3cpp::services::SceneSource GetSceneSource() const override {
        return sdl3cpp::services::SceneSource::Lua;
    }
    const sdl3cpp::services::InputBindings& GetInputBindings() const override { return inputBindings_; }
    const sdl3cpp::services::MouseGrabConfig& GetMouseGrabConfig() const override { return mouseGrabConfig_; }
    const sdl3cpp::services::BgfxConfig& GetBgfxConfig() const override { return bgfxConfig_; }
    const sdl3cpp::services::MaterialXConfig& GetMaterialXConfig() const override { return materialXConfig_; }
    const std::vector<sdl3cpp::services::MaterialXMaterialConfig>& GetMaterialXMaterialConfigs() const override {
        return materialXMaterials_;
    }
    const sdl3cpp::services::GuiFontConfig& GetGuiFontConfig() const override { return guiFontConfig_; }
    const sdl3cpp::services::RenderBudgetConfig& GetRenderBudgetConfig() const override { return budgets_; }
    const sdl3cpp::services::CrashRecoveryConfig& GetCrashRecoveryConfig() const override { return crashRecovery_; }
    const sdl3cpp::services::ValidationTourConfig& GetValidationTourConfig() const override {
        return validationTour_;
    }
    const std::string& GetConfigJson() const override { return configJson_; }

private:
    static std::filesystem::path ResolveMaterialXLibraryPath() {
        auto repoRoot = std::filesystem::path(__FILE__).parent_path().parent_path();
        return repoRoot / "MaterialX" / "libraries";
    }

    std::filesystem::path scriptPath_;
    std::string configJson_;
    sdl3cpp::services::InputBindings inputBindings_{};
    sdl3cpp::services::MouseGrabConfig mouseGrabConfig_{};
    sdl3cpp::services::BgfxConfig bgfxConfig_{};
    sdl3cpp::services::MaterialXConfig materialXConfig_{};
    std::vector<sdl3cpp::services::MaterialXMaterialConfig> materialXMaterials_{};
    sdl3cpp::services::GuiFontConfig guiFontConfig_{};
    sdl3cpp::services::RenderBudgetConfig budgets_{};
    sdl3cpp::services::CrashRecoveryConfig crashRecovery_{};
    sdl3cpp::services::ValidationTourConfig validationTour_{};
    std::string renderer_;
};

class StubAudioCommandService final : public sdl3cpp::services::IAudioCommandService {
public:
    bool QueueAudioCommand(sdl3cpp::services::AudioCommandType,
                           const std::string&,
                           bool,
                           std::string&) override {
        return true;
    }

    bool StopBackground(std::string&) override {
        return true;
    }
};

void Assert(bool condition, const std::string& message, int& failures) {
    if (!condition) {
        std::cerr << "test failure: " << message << '\n';
        ++failures;
    }
}

struct MatrixSummary {
    std::array<float, 3> translation{};
    std::array<float, 3> scale{};
};

MatrixSummary ExtractMatrixSummary(const std::array<float, 16>& matrix) {
    MatrixSummary summary;
    summary.translation = {matrix[12], matrix[13], matrix[14]};
    
    // Extract scale as magnitude of basis vectors (correct for rotation+scale matrices)
    // X-axis: matrix[0,1,2], Y-axis: matrix[4,5,6], Z-axis: matrix[8,9,10]
    auto length = [](float x, float y, float z) {
        return std::sqrt(x*x + y*y + z*z);
    };
    summary.scale = {
        length(matrix[0], matrix[1], matrix[2]),
        length(matrix[4], matrix[5], matrix[6]),
        length(matrix[8], matrix[9], matrix[10])
    };
    
    return summary;
}

std::array<float, 16> ToArray(const glm::mat4& matrix) {
    std::array<float, 16> values{};
    std::memcpy(values.data(), glm::value_ptr(matrix), sizeof(float) * values.size());
    return values;
}

bool ExpectMatrixNear(const std::array<float, 16>& actual,
                      const std::array<float, 16>& expected,
                      const std::string& label,
                      int& failures,
                      float eps = 1e-4f) {
    for (size_t i = 0; i < actual.size(); ++i) {
        if (!ApproximatelyEqual(actual[i], expected[i], eps)) {
            std::cerr << label << " differs at index " << i << " (" << actual[i]
                      << " vs " << expected[i] << ")\n";
            ++failures;
            return false;
        }
    }
    return true;
}

bool ExpectColorNear(const sdl3cpp::core::Vertex& vertex,
                     const std::array<float, 3>& expected,
                     const std::string& label,
                     int& failures,
                     float eps = 1e-4f) {
    for (size_t i = 0; i < expected.size(); ++i) {
        if (!ApproximatelyEqual(vertex.color[i], expected[i], eps)) {
            std::cerr << label << " color differs at index " << i << " (" << vertex.color[i]
                      << " vs " << expected[i] << ")\n";
            ++failures;
            return false;
        }
    }
    return true;
}

bool RunGpuRenderTest(int& failures,
                      const std::shared_ptr<sdl3cpp::services::IConfigService>& configService,
                      const std::shared_ptr<sdl3cpp::services::ILogger>& logger) {
    const char* preferredDrivers[] = {"x11", "wayland", "offscreen", "dummy", nullptr};
    const char* selectedDriver = nullptr;
    
    auto setVideoDriver = [](const char* driver) {
#ifdef _WIN32
        if (driver) {
            _putenv_s("SDL_VIDEODRIVER", driver);
        } else {
            _putenv_s("SDL_VIDEODRIVER", "");
        }
#else
        if (driver) {
            setenv("SDL_VIDEODRIVER", driver, 1);
        } else {
            unsetenv("SDL_VIDEODRIVER");
        }
#endif
        if (driver) {
            SDL_SetHint(SDL_HINT_VIDEO_DRIVER, driver);
        } else {
            SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "");
        }
    };

    auto platformService = std::make_shared<sdl3cpp::services::impl::PlatformService>(logger);
    auto eventBus = std::make_shared<sdl3cpp::events::EventBus>();
    auto windowService = std::make_shared<sdl3cpp::services::impl::SdlWindowService>(logger, platformService, eventBus);
    
    SDL_Window* window = nullptr;
    bool windowCreated = false;

    for (const char* driver : preferredDrivers) {
        setVideoDriver(driver);
        SDL_ClearError();
        
        try {
            windowService->Initialize();
            sdl3cpp::services::WindowConfig config{};
            config.width = 256;
            config.height = 256;
            config.title = "cube_gpu_test";
            config.resizable = false;
            windowService->CreateWindow(config);
            window = windowService->GetNativeHandle();
            windowCreated = true;
            selectedDriver = driver;
            break;
        } catch (const std::exception& ex) {
            std::cerr << "Window creation failed for driver [" << (driver ? driver : "default")
                      << "]: " << ex.what() << '\n';
            windowService->Shutdown();
            windowService = std::make_shared<sdl3cpp::services::impl::SdlWindowService>(logger, platformService, eventBus);
        }
    }

    if (!windowCreated) {
        std::string driverList;
        const int driverCount = SDL_GetNumVideoDrivers();
        for (int i = 0; i < driverCount; ++i) {
            const char* driver = SDL_GetVideoDriver(i);
            if (!driver) {
                continue;
            }
            if (!driverList.empty()) {
                driverList += ", ";
            }
            driverList += driver;
        }
        if (driverList.empty()) {
            driverList = "none";
        }
        std::cerr << "GPU render test failed: no SDL driver available"
                  << " (available drivers: " << driverList << ")\n";
        ++failures;
        return false;
    }

    if (selectedDriver) {
        std::cout << "SDL video driver selected for GPU test: " << selectedDriver << '\n';
    } else {
        std::cout << "SDL video driver selected for GPU test: default\n";
    }

    auto pipelineCompiler = std::make_shared<sdl3cpp::services::impl::PipelineCompilerService>(logger);
    sdl3cpp::services::impl::BgfxGraphicsBackend backend(configService, platformService, logger, pipelineCompiler);
    bool success = true;
    bool skipGpuRenderTest = false;
    
    try {
        sdl3cpp::services::GraphicsConfig graphicsConfig{};
        backend.Initialize(window, graphicsConfig);
    } catch (const std::exception& ex) {
        std::cerr << "GPU render test failed: bgfx init threw: " << ex.what() << '\n';
        ++failures;
        success = false;
    }

    if (success && bgfx::getRendererType() == bgfx::RendererType::Noop) {
        const std::string activeDriver = selectedDriver ? selectedDriver : "";
        if (activeDriver == "offscreen" || activeDriver == "dummy") {
            std::cerr << "GPU render test skipped: bgfx selected Noop renderer for headless driver '" <<
                         activeDriver << "'\n";
            skipGpuRenderTest = true;
        } else {
            std::cerr << "GPU render test failed: bgfx selected Noop renderer despite SDL success\n";
            ++failures;
            success = false;
        }
    }

    if (success && !skipGpuRenderTest) {
        std::cout << "GPU render test: Validating full render pipeline with scene geometry\n";

        // Load and render the actual cube scene to catch color, geometry, and animation issues
        auto scriptPath = GetCubeScriptPath();
        auto sceneConfigService = configService;
        auto meshService = std::make_shared<sdl3cpp::services::impl::MeshService>(sceneConfigService, logger);
        auto audioService = std::make_shared<StubAudioCommandService>();
        auto physicsService = std::make_shared<sdl3cpp::services::impl::PhysicsBridgeService>(logger);

        auto engineService = std::make_shared<sdl3cpp::services::impl::ScriptEngineService>(
            scriptPath,
            logger,
            meshService,
            audioService,
            physicsService,
            nullptr,
            nullptr,
            sceneConfigService,
            false);
        engineService->Initialize();

        auto sceneScriptService = std::make_shared<sdl3cpp::services::impl::SceneScriptService>(engineService, logger);
        auto objects = sceneScriptService->LoadSceneObjects();

        if (objects.size() != 15) {
            std::cerr << "GPU render test: Scene loaded " << objects.size() << " objects, expected 15\n";
            ++failures;
            success = false;
        }

        // Validate all geometry is present
        bool hasFloor = false;
        bool hasCeiling = false;
        int wallCount = 0;
        int lanternCount = 0;
        bool hasCube = false;

        for (const auto& obj : objects) {
            const auto& type = obj.objectType;

            if (type == "floor") {
                hasFloor = true;
            } else if (type == "ceiling") {
                hasCeiling = true;
            } else if (type == "wall") {
                wallCount++;
            } else if (type == "lantern") {
                lanternCount++;
            } else if (type == "physics_cube" || type == "spinning_cube") {
                hasCube = true;
            }
        }

        Assert(hasFloor, "GPU render test: Missing floor geometry", failures);
        Assert(hasCeiling, "GPU render test: Missing ceiling geometry", failures);
        Assert(wallCount == 4, "GPU render test: Expected 4 walls, got " + std::to_string(wallCount), failures);
        Assert(lanternCount == 8, "GPU render test: Expected 8 lanterns, got " + std::to_string(lanternCount), failures);
        Assert(hasCube, "GPU render test: Missing physics cube geometry", failures);

        // Validate all scene objects have valid shader keys (critical for rendering)
        for (size_t i = 0; i < objects.size(); ++i) {
            const auto& obj = objects[i];
            Assert(!obj.shaderKeys.empty(), 
                   "GPU render test: Object " + std::to_string(i) + " (" + obj.objectType + ") has no shader keys", 
                   failures);

            // Validate room geometry (floor, ceiling, walls) has expected shader keys
            if (obj.objectType == "floor") {
                Assert(!obj.shaderKeys.empty() && obj.shaderKeys.front() == "floor",
                       "GPU render test: Floor should have shader key 'floor'", failures);
                Assert(obj.vertices.size() >= 100,
                       "GPU render test: Floor should have tessellated geometry (expected >= 100 vertices, got " + 
                       std::to_string(obj.vertices.size()) + ")", failures);
            } else if (obj.objectType == "ceiling") {
                Assert(!obj.shaderKeys.empty() && obj.shaderKeys.front() == "ceiling",
                       "GPU render test: Ceiling should have shader key 'ceiling'", failures);
                Assert(obj.vertices.size() >= 100,
                       "GPU render test: Ceiling should have tessellated geometry (expected >= 100 vertices, got " + 
                       std::to_string(obj.vertices.size()) + ")", failures);
            } else if (obj.objectType == "wall") {
                Assert(!obj.shaderKeys.empty() && obj.shaderKeys.front() == "wall",
                       "GPU render test: Wall should have shader key 'wall'", failures);
            }
        }

        // Create actual render buffers and render multiple frames to test animation
        auto ecsService = std::make_shared<sdl3cpp::services::impl::EcsService>(logger);
        auto sceneService = std::make_shared<sdl3cpp::services::impl::SceneService>(sceneScriptService, ecsService, logger);
        sceneService->LoadScene(objects);

        auto device = backend.CreateDevice();
        const int testFrames = 5;
        std::cout << "GPU render test: Rendering " << testFrames << " frames to validate pipeline\n";

        for (int frame = 0; frame < testFrames; ++frame) {
            float elapsedTime = frame * 0.016f;  // ~60 FPS
            auto renderCommands = sceneService->GetRenderCommands(elapsedTime);

            if (renderCommands.size() != objects.size()) {
                std::cerr << "GPU render test: Frame " << frame << " produced " 
                          << renderCommands.size() << " commands, expected " << objects.size() << '\n';
                ++failures;
                success = false;
                break;
            }

            backend.BeginFrame(device);

            // Validate cube is animating (matrix should change between frames)
            if (frame == 0) {
                for (size_t i = 0; i < renderCommands.size(); ++i) {
                    if (objects[i].shaderKeys.empty()) continue;
                    const auto& key = objects[i].shaderKeys.front();
                    if (key == "floor" && objects[i].vertices.size() < 100) {
                        // This is the cube - verify it has non-identity matrix (spinning)
                        bool hasRotation = false;
                        const auto& matrix = renderCommands[i].modelMatrix;
                        // Check off-diagonal elements for rotation
                        if (std::abs(matrix[1]) > 0.01f || std::abs(matrix[2]) > 0.01f || 
                            std::abs(matrix[4]) > 0.01f || std::abs(matrix[6]) > 0.01f ||
                            std::abs(matrix[8]) > 0.01f || std::abs(matrix[9]) > 0.01f) {
                            hasRotation = true;
                        }
                        if (!hasRotation) {
                            std::cerr << "GPU render test: Cube is not spinning (rotation matrix is identity)\n";
                            ++failures;
                            success = false;
                        }
                        break;
                    }
                }
            }

            backend.EndFrame(device);
        }

        backend.DestroyDevice(device);
        sceneService->Shutdown();
        engineService->Shutdown();

        std::cout << "GPU render test: Successfully rendered and validated scene pipeline\n";
    }
    
    backend.Shutdown();
    windowService->DestroyWindow();
    windowService->Shutdown();
    
    return success;
}

void RunCubeDemoSceneTests(int& failures) {
    auto scriptPath = GetCubeScriptPath();
    auto configPath = GetSeedConfigPath();
    auto configJson = ReadFileContents(configPath);
    Assert(static_cast<bool>(configJson), "seed runtime config missing", failures);
    if (!configJson) {
        return;
    }

    auto logger = std::make_shared<sdl3cpp::services::impl::LoggerService>();
    const std::string preferredRenderer = DeterminePreferredRenderer();
    auto configService = std::make_shared<CubeDemoConfigService>(scriptPath, *configJson, preferredRenderer);
    auto meshService = std::make_shared<sdl3cpp::services::impl::MeshService>(configService, logger);
    auto audioService = std::make_shared<StubAudioCommandService>();
    auto physicsService = std::make_shared<sdl3cpp::services::impl::PhysicsBridgeService>(logger);

    if (!RunGpuRenderTest(failures, configService, logger)) {
        std::cerr << "Aborting cube scene checks because GPU render test failed\n";
        return;
    }

    auto engineService = std::make_shared<sdl3cpp::services::impl::ScriptEngineService>(
        scriptPath,
        logger,
        meshService,
        audioService,
        physicsService,
        nullptr,
        nullptr,
        configService,
        false);
    engineService->Initialize();

    auto sceneScriptService = std::make_shared<sdl3cpp::services::impl::SceneScriptService>(engineService, logger);
    auto objects = sceneScriptService->LoadSceneObjects();

    Assert(objects.size() == 15, "cube demo should return 15 scene objects", failures);
    if (objects.empty()) {
        engineService->Shutdown();
        return;
    }

    for (const auto& object : objects) {
        Assert(!object.vertices.empty(), "scene object missing vertices", failures);
        Assert(!object.indices.empty(), "scene object missing indices", failures);
        Assert(!object.shaderKeys.empty(), "scene object missing shader key", failures);
        Assert(object.computeModelMatrixRef >= 0, "scene object must keep a Lua reference", failures);
    }

    auto ecsService = std::make_shared<sdl3cpp::services::impl::EcsService>(logger);
    auto sceneManager = std::make_shared<sdl3cpp::services::impl::SceneService>(sceneScriptService, ecsService, logger);
    sceneManager->LoadScene(objects);
    Assert(sceneManager->GetObjectCount() == objects.size(), "scene service object count mismatch", failures);

    size_t expectedVertexCount = 0;
    size_t expectedIndexCount = 0;
    for (const auto& object : objects) {
        expectedVertexCount += object.vertices.size();
        expectedIndexCount += object.indices.size();
    }

    const auto& combinedVertices = sceneManager->GetCombinedVertices();
    const auto& combinedIndices = sceneManager->GetCombinedIndices();
    Assert(combinedVertices.size() == expectedVertexCount, "combined vertex count mismatch", failures);
    Assert(combinedIndices.size() == expectedIndexCount, "combined index count mismatch", failures);

    const std::array<float, 3> white = {1.0f, 1.0f, 1.0f};
    const std::array<float, 3> lanternColor = {1.0f, 0.9f, 0.6f};
    const std::array<float, 3> cubeColor = {0.92f, 0.34f, 0.28f};

    const float roomHalfSize = 15.0f;
    const float wallThickness = 0.5f;
    const float wallHeight = 4.0f;
    const float floorHalfThickness = 0.3f;
    const float floorTop = 0.0f;
    const float floorCenterY = floorTop - floorHalfThickness;
    const float wallCenterY = floorTop + wallHeight;
    const float ceilingY = floorTop + wallHeight * 2.0f + floorHalfThickness;
    const float wallOffset = roomHalfSize + wallThickness;
    const float lanternHeight = 8.0f;
    const float lanternSize = 0.2f;
    const float lanternOffset = roomHalfSize - 2.0f;
    const float cubeSpawnY = floorTop + wallHeight + 1.5f + 0.5f;

    auto staticCommands = sceneManager->GetRenderCommands(0.0f);
    auto dynamicCommands = sceneManager->GetRenderCommands(0.1f);
    Assert(staticCommands.size() == objects.size(), "render command count mismatch", failures);
    Assert(dynamicCommands.size() == objects.size(), "dynamic render command count mismatch", failures);

    std::vector<size_t> floorIndices;
    std::vector<size_t> wallIndices;
    std::vector<size_t> ceilingIndices;
    std::vector<size_t> solidIndices;
    std::vector<size_t> otherIndices;
    std::vector<std::array<float, 3>> wallTranslations;
    wallTranslations.reserve(4);
    std::vector<std::array<float, 3>> lanternTranslations;
    lanternTranslations.reserve(8);

    for (size_t index = 0; index < staticCommands.size(); ++index) {
        const auto& command = staticCommands[index];
        const auto& object = objects[index];
        Assert(!command.shaderKeys.empty(), "scene object missing shader key", failures);
        Assert(!object.indices.empty(), "scene object should have indices", failures);

        const auto summary = ExtractMatrixSummary(command.modelMatrix);
        const std::string& objectType = object.objectType;
        
        if (objectType == "floor") {
            floorIndices.push_back(index);
            if (!object.vertices.empty()) {
                ExpectColorNear(object.vertices.front(), white, "floor vertex color", failures);
            }
        } else if (objectType == "wall") {
            wallIndices.push_back(index);
            wallTranslations.push_back(summary.translation);
            Assert(ApproximatelyEqual(summary.scale[1], wallHeight), "wall scale height mismatch", failures);
            if (!object.vertices.empty()) {
                ExpectColorNear(object.vertices.front(), white, "wall vertex color", failures);
            }
        } else if (objectType == "ceiling") {
            ceilingIndices.push_back(index);
            Assert(ApproximatelyEqual(summary.translation[1], ceilingY), "ceiling translation mismatch", failures);
            // Ceiling now uses tessellated plane with scale 1.0 (geometry is pre-sized)
            Assert(ApproximatelyEqual(summary.scale[0], 1.0f, 0.1f), "ceiling scale mismatch", failures);
            if (!object.vertices.empty()) {
                ExpectColorNear(object.vertices.front(), white, "ceiling vertex color", failures);
            }
        } else if (objectType == "lantern") {
            solidIndices.push_back(index);
            lanternTranslations.push_back(summary.translation);
            Assert(ApproximatelyEqual(summary.scale[0], lanternSize), "lantern scale mismatch", failures);
            if (!object.vertices.empty()) {
                ExpectColorNear(object.vertices.front(), lanternColor, "lantern vertex color", failures);
            }
        } else if (objectType == "physics_cube" || objectType == "spinning_cube") {
            // Physics cube is tracked separately below
        } else {
            otherIndices.push_back(index);
        }
    }

    Assert(ceilingIndices.size() == 1, "expected 1 ceiling object", failures);
    Assert(wallIndices.size() == 4, "expected 4 wall objects", failures);
    Assert(solidIndices.size() == 8, "expected 8 lantern objects", failures);
    Assert(floorIndices.size() == 1, "expected 1 floor object", failures);
    Assert(otherIndices.empty(), "unexpected object types in cube demo scene", failures);

    const std::vector<std::array<float, 3>> expectedWallTranslations = {
        {0.0f, wallCenterY, -wallOffset},
        {0.0f, wallCenterY, wallOffset},
        {-wallOffset, wallCenterY, 0.0f},
        {wallOffset, wallCenterY, 0.0f},
    };
    for (const auto& expected : expectedWallTranslations) {
        bool found = false;
        for (const auto& actual : wallTranslations) {
            if (ApproximatelyEqual(actual[0], expected[0])
                && ApproximatelyEqual(actual[1], expected[1])
                && ApproximatelyEqual(actual[2], expected[2])) {
                found = true;
                break;
            }
        }
        Assert(found, "missing wall at expected translation", failures);
    }

    const std::vector<std::array<float, 3>> expectedLanternTranslations = {
        {lanternOffset, lanternHeight, lanternOffset},
        {-lanternOffset, lanternHeight, lanternOffset},
        {lanternOffset, lanternHeight, -lanternOffset},
        {-lanternOffset, lanternHeight, -lanternOffset},
        {0.0f, lanternHeight, lanternOffset},
        {0.0f, lanternHeight, -lanternOffset},
        {lanternOffset, lanternHeight, 0.0f},
        {-lanternOffset, lanternHeight, 0.0f},
    };
    for (const auto& expected : expectedLanternTranslations) {
        bool found = false;
        for (const auto& actual : lanternTranslations) {
            if (ApproximatelyEqual(actual[0], expected[0])
                && ApproximatelyEqual(actual[1], expected[1])
                && ApproximatelyEqual(actual[2], expected[2])) {
                found = true;
                break;
            }
        }
        Assert(found, "missing lantern at expected translation", failures);
    }

    // Find floor and physics cube by object type
    size_t floorObjectIndex = std::numeric_limits<size_t>::max();
    size_t cubeObjectIndex = std::numeric_limits<size_t>::max();
    
    for (size_t idx = 0; idx < objects.size(); ++idx) {
        if (objects[idx].objectType == "floor") {
            floorObjectIndex = idx;
        } else if (objects[idx].objectType == "physics_cube" || objects[idx].objectType == "spinning_cube") {
            cubeObjectIndex = idx;
        }
    }
    
    Assert(floorObjectIndex != std::numeric_limits<size_t>::max(), "floor object not found", failures);
    Assert(cubeObjectIndex != std::numeric_limits<size_t>::max(), "dynamic cube object not found", failures);

    if (floorObjectIndex != std::numeric_limits<size_t>::max()) {
        auto summary = ExtractMatrixSummary(staticCommands[floorObjectIndex].modelMatrix);
        Assert(ApproximatelyEqual(summary.translation[1], floorCenterY), "floor translation mismatch", failures);
        // Floor now has scale 1.0 (geometry is pre-sized)
        Assert(ApproximatelyEqual(summary.scale[0], 1.0f, 0.1f), "floor scale mismatch", failures);
        if (!objects[floorObjectIndex].vertices.empty()) {
            ExpectColorNear(objects[floorObjectIndex].vertices.front(), white, "floor vertex color", failures);
        }
        Assert(!objects[floorObjectIndex].indices.empty(), "floor indices should not be empty", failures);
    }

    if (cubeObjectIndex != std::numeric_limits<size_t>::max()) {
        auto summary = ExtractMatrixSummary(dynamicCommands[cubeObjectIndex].modelMatrix);
        Assert(ApproximatelyEqual(summary.translation[0], 0.0f, 0.05f),
               "physics cube x translation mismatch", failures);
        Assert(ApproximatelyEqual(summary.translation[2], 0.0f, 0.05f),
               "physics cube z translation mismatch", failures);
        Assert(ApproximatelyEqual(summary.translation[1], cubeSpawnY, 0.25f),
               "physics cube y translation mismatch", failures);
        // Physics cube scale is 1.5, now correctly extracted from rotated matrix
        Assert(ApproximatelyEqual(summary.scale[0], 1.5f, 0.1f), "physics cube scale mismatch", failures);
        if (!objects[cubeObjectIndex].vertices.empty()) {
            ExpectColorNear(objects[cubeObjectIndex].vertices.front(), cubeColor, "physics cube vertex color", failures);
        }
        Assert(!objects[cubeObjectIndex].indices.empty(), "cube indices should not be empty", failures);
    }

    sceneManager->Shutdown();
    engineService->Shutdown();
}
} // namespace

// When invoking this test locally, prefer `python scripts/dev_commands.py cmake -- --target script_engine_tests`
// so the helper picks the right build dir/generator.
int main() {
    int failures = 0;
    auto scriptPath = GetTestScriptPath();
    std::cout << "Loading Lua fixture: " << scriptPath << '\n';

    try {
        auto logger = std::make_shared<sdl3cpp::services::impl::LoggerService>();
        auto configService = std::make_shared<StubConfigService>();
        auto engineService = std::make_shared<sdl3cpp::services::impl::ScriptEngineService>(
            scriptPath,
            logger,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            configService,
            false);
        engineService->Initialize();

        sdl3cpp::services::impl::SceneScriptService sceneService(engineService, logger);
        auto shaderSystemRegistry = std::make_shared<sdl3cpp::services::impl::ShaderSystemRegistry>(
            configService,
            nullptr,
            engineService,
            logger);
        sdl3cpp::services::impl::ShaderScriptService shaderService(
            engineService,
            shaderSystemRegistry,
            logger);

        auto objects = sceneService.LoadSceneObjects();
        Assert(objects.size() == 1, "expected exactly one scene object", failures);
        if (!objects.empty()) {
            const auto& object = objects.front();
            Assert(object.vertices.size() == 3, "scene object should yield three vertices", failures);
            Assert(object.indices.size() == 3, "scene object should yield three indices", failures);
            Assert(object.shaderKeys.size() == 1, "shader keys should contain one entry", failures);
            if (!object.shaderKeys.empty()) {
                Assert(object.shaderKeys.front() == "test", "shader key should match fixture", failures);
            }
            const std::vector<uint16_t> expectedIndices{0, 1, 2};
            Assert(object.indices == expectedIndices, "indices should be zero-based", failures);
            Assert(object.computeModelMatrixRef >= 0,
                   "vertex object must keep a Lua reference", failures);

            auto objectMatrix = sceneService.ComputeModelMatrix(object.computeModelMatrixRef, 0.5f);
            ExpectIdentity(objectMatrix, "object compute_model_matrix", failures);
        }

        auto fallbackMatrix = sceneService.ComputeModelMatrix(-1, 1.0f);
        ExpectIdentity(fallbackMatrix, "global compute_model_matrix", failures);

        auto viewState = sceneService.GetViewState(1.33f);
        ExpectIdentity(viewState.viewProj, "view_projection matrix", failures);

        auto shaderMap = shaderService.LoadShaderPathsMap();
        Assert(shaderMap.size() == 1, "expected a single shader variant", failures);
        auto testEntry = shaderMap.find("test");
        Assert(testEntry != shaderMap.end(), "shader map missing test entry", failures);
        if (testEntry != shaderMap.end()) {
            Assert(!testEntry->second.vertexSource.empty(), "vertex shader source missing", failures);
            Assert(!testEntry->second.fragmentSource.empty(), "fragment shader source missing", failures);
        }

        RunCubeDemoSceneTests(failures);
    } catch (const std::exception& ex) {
        std::cerr << "exception during tests: " << ex.what() << '\n';
        return 1;
    }

    if (failures == 0) {
        std::cout << "script_engine_tests: PASSED\n";
    } else {
        std::cerr << "script_engine_tests: FAILED (" << failures << " errors)\n";
    }

    return failures;
}
