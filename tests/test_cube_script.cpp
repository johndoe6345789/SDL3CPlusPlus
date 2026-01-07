#include "services/impl/bgfx_graphics_backend.hpp"
#include "services/impl/logger_service.hpp"
#include "services/impl/mesh_service.hpp"
#include "services/impl/physics_bridge_service.hpp"
#include "services/impl/platform_service.hpp"
#include "services/impl/script_engine_service.hpp"
#include "services/impl/scene_script_service.hpp"
#include "services/impl/shader_script_service.hpp"
#include "services/interfaces/i_audio_command_service.hpp"
#include "services/interfaces/i_config_service.hpp"

#include <array>
#include <bgfx/bgfx.h>
#include <cmath>
#include <cstring>
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
    const sdl3cpp::services::InputBindings& GetInputBindings() const override { return inputBindings_; }
    const sdl3cpp::services::MouseGrabConfig& GetMouseGrabConfig() const override { return mouseGrabConfig_; }
    const sdl3cpp::services::BgfxConfig& GetBgfxConfig() const override { return bgfxConfig_; }
    const sdl3cpp::services::MaterialXConfig& GetMaterialXConfig() const override { return materialXConfig_; }
    const std::vector<sdl3cpp::services::MaterialXMaterialConfig>& GetMaterialXMaterialConfigs() const override {
        return materialXMaterials_;
    }
    const sdl3cpp::services::GuiFontConfig& GetGuiFontConfig() const override { return guiFontConfig_; }
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
    std::string configJson_{};
};

class CubeDemoConfigService final : public sdl3cpp::services::IConfigService {
public:
    CubeDemoConfigService(std::filesystem::path scriptPath, std::string configJson)
        : scriptPath_(std::move(scriptPath)),
          configJson_(std::move(configJson)) {
        materialXConfig_.enabled = true;
        materialXConfig_.useConstantColor = true;
        materialXConfig_.shaderKey = "test";
        materialXConfig_.libraryPath = ResolveMaterialXLibraryPath();
        bgfxConfig_.renderer = "auto";
    }

    uint32_t GetWindowWidth() const override { return 1; }
    uint32_t GetWindowHeight() const override { return 1; }
    std::filesystem::path GetScriptPath() const override { return scriptPath_; }
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
    summary.scale = {matrix[0], matrix[5], matrix[10]};
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

void RunGpuSmokeTest(int& failures,
                     const std::shared_ptr<sdl3cpp::services::IConfigService>& configService,
                     const std::shared_ptr<sdl3cpp::services::ILogger>& logger) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "test failure: SDL video init failed: " << SDL_GetError() << '\n';
        ++failures;
        return;
    }

    SDL_Window* window = SDL_CreateWindow("cube_gpu_test", 64, 64, SDL_WINDOW_HIDDEN);
    if (!window) {
        std::cerr << "test failure: SDL window creation failed: " << SDL_GetError() << '\n';
        ++failures;
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return;
    }

    auto platformService = std::make_shared<sdl3cpp::services::impl::PlatformService>(logger);
    sdl3cpp::services::impl::BgfxGraphicsBackend backend(configService, platformService, logger);
    try {
        sdl3cpp::services::GraphicsConfig graphicsConfig{};
        backend.Initialize(window, graphicsConfig);
    } catch (const std::exception& ex) {
        std::cerr << "test failure: bgfx init threw: " << ex.what() << '\n';
        ++failures;
        SDL_DestroyWindow(window);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return;
    }

    Assert(bgfx::getRendererType() != bgfx::RendererType::Noop,
           "bgfx selected Noop renderer; GPU unavailable", failures);

    auto device = backend.CreateDevice();
    backend.BeginFrame(device);
    backend.EndFrame(device);
    backend.DestroyDevice(device);
    backend.Shutdown();

    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
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
    auto configService = std::make_shared<CubeDemoConfigService>(scriptPath, *configJson);
    auto meshService = std::make_shared<sdl3cpp::services::impl::MeshService>(configService, logger);
    auto audioService = std::make_shared<StubAudioCommandService>();
    auto physicsService = std::make_shared<sdl3cpp::services::impl::PhysicsBridgeService>(logger);

    RunGpuSmokeTest(failures, configService, logger);
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

    sdl3cpp::services::impl::SceneScriptService sceneService(engineService, logger);
    auto objects = sceneService.LoadSceneObjects();

    Assert(objects.size() == 16, "cube demo should return 16 scene objects", failures);

    std::vector<const sdl3cpp::services::SceneObject*> floorObjects;
    std::vector<const sdl3cpp::services::SceneObject*> wallObjects;
    std::vector<const sdl3cpp::services::SceneObject*> ceilingObjects;
    std::vector<const sdl3cpp::services::SceneObject*> solidObjects;
    std::vector<const sdl3cpp::services::SceneObject*> skyboxObjects;
    std::vector<const sdl3cpp::services::SceneObject*> otherObjects;

    for (const auto& object : objects) {
        Assert(!object.shaderKeys.empty(), "scene object missing shader key", failures);
        if (object.shaderKeys.empty()) {
            continue;
        }
        const std::string& shaderKey = object.shaderKeys.front();
        if (shaderKey == "floor") {
            floorObjects.push_back(&object);
        } else if (shaderKey == "wall") {
            wallObjects.push_back(&object);
        } else if (shaderKey == "ceiling") {
            ceilingObjects.push_back(&object);
        } else if (shaderKey == "solid") {
            solidObjects.push_back(&object);
        } else if (shaderKey == "skybox") {
            skyboxObjects.push_back(&object);
        } else {
            otherObjects.push_back(&object);
        }
    }

    Assert(ceilingObjects.size() == 1, "expected 1 ceiling object", failures);
    Assert(wallObjects.size() == 4, "expected 4 wall objects", failures);
    Assert(solidObjects.size() == 8, "expected 8 lantern objects", failures);
    Assert(skyboxObjects.size() == 1, "expected 1 skybox object", failures);
    Assert(floorObjects.size() == 2, "expected 2 floor-key objects (floor + cube)", failures);
    Assert(otherObjects.empty(), "unexpected shader keys in cube demo scene", failures);

    const std::array<float, 3> white = {1.0f, 1.0f, 1.0f};
    const std::array<float, 3> lanternColor = {1.0f, 0.9f, 0.6f};
    const std::array<float, 3> skyboxColor = {0.04f, 0.05f, 0.08f};
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

    std::vector<std::array<float, 3>> wallTranslations;
    wallTranslations.reserve(wallObjects.size());
    for (const auto* object : wallObjects) {
        auto matrix = sceneService.ComputeModelMatrix(object->computeModelMatrixRef, 0.0f);
        auto summary = ExtractMatrixSummary(matrix);
        wallTranslations.push_back(summary.translation);
        Assert(ApproximatelyEqual(summary.scale[1], wallHeight), "wall scale height mismatch", failures);
        Assert(!object->indices.empty(), "wall indices should not be empty", failures);
        if (!object->vertices.empty()) {
            ExpectColorNear(object->vertices.front(), white, "wall vertex color", failures);
        }
    }

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

    if (!ceilingObjects.empty()) {
        const auto* ceiling = ceilingObjects.front();
        auto matrix = sceneService.ComputeModelMatrix(ceiling->computeModelMatrixRef, 0.0f);
        auto summary = ExtractMatrixSummary(matrix);
        Assert(ApproximatelyEqual(summary.translation[1], ceilingY), "ceiling translation mismatch", failures);
        Assert(ApproximatelyEqual(summary.scale[1], floorHalfThickness), "ceiling thickness mismatch", failures);
        Assert(!ceiling->indices.empty(), "ceiling indices should not be empty", failures);
        if (!ceiling->vertices.empty()) {
            ExpectColorNear(ceiling->vertices.front(), white, "ceiling vertex color", failures);
        }
    }

    const sdl3cpp::services::SceneObject* floorObject = nullptr;
    const sdl3cpp::services::SceneObject* cubeObject = nullptr;
    for (const auto* object : floorObjects) {
        auto matrix = sceneService.ComputeModelMatrix(object->computeModelMatrixRef, 0.0f);
        auto summary = ExtractMatrixSummary(matrix);
        if (ApproximatelyEqual(summary.scale[0], roomHalfSize)
            && ApproximatelyEqual(summary.scale[2], roomHalfSize)) {
            floorObject = object;
        } else if (ApproximatelyEqual(summary.scale[0], 1.5f)) {
            cubeObject = object;
        }
    }
    Assert(floorObject != nullptr, "floor object not found", failures);
    Assert(cubeObject != nullptr, "dynamic cube object not found", failures);

    if (floorObject) {
        auto matrix = sceneService.ComputeModelMatrix(floorObject->computeModelMatrixRef, 0.0f);
        auto summary = ExtractMatrixSummary(matrix);
        Assert(ApproximatelyEqual(summary.translation[1], floorCenterY), "floor translation mismatch", failures);
        Assert(ApproximatelyEqual(summary.scale[1], floorHalfThickness), "floor thickness mismatch", failures);
        Assert(!floorObject->indices.empty(), "floor indices should not be empty", failures);
        if (!floorObject->vertices.empty()) {
            ExpectColorNear(floorObject->vertices.front(), white, "floor vertex color", failures);
        }
    }

    if (cubeObject) {
        const float time = 0.1f;
        auto matrix = sceneService.ComputeModelMatrix(cubeObject->computeModelMatrixRef, time);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, cubeSpawnY, 0.0f));
        glm::mat4 rotation = glm::mat4_cast(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(1.5f, 1.5f, 1.5f));
        glm::mat4 expected = translation * rotation * scale;
        ExpectMatrixNear(matrix, ToArray(expected), "physics cube matrix at t=0.1", failures);
        Assert(!cubeObject->indices.empty(), "cube indices should not be empty", failures);
        if (!cubeObject->vertices.empty()) {
            ExpectColorNear(cubeObject->vertices.front(), cubeColor, "physics cube vertex color", failures);
        }
    }

    std::vector<std::array<float, 3>> lanternTranslations;
    lanternTranslations.reserve(solidObjects.size());
    for (const auto* object : solidObjects) {
        auto matrix = sceneService.ComputeModelMatrix(object->computeModelMatrixRef, 0.0f);
        auto summary = ExtractMatrixSummary(matrix);
        lanternTranslations.push_back(summary.translation);
        Assert(ApproximatelyEqual(summary.scale[0], lanternSize), "lantern scale mismatch", failures);
        if (!object->vertices.empty()) {
            ExpectColorNear(object->vertices.front(), lanternColor, "lantern vertex color", failures);
        }
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

    if (!skyboxObjects.empty()) {
        const auto* skybox = skyboxObjects.front();
        auto matrix = sceneService.ComputeModelMatrix(skybox->computeModelMatrixRef, 0.0f);
        auto summary = ExtractMatrixSummary(matrix);
        Assert(ApproximatelyEqual(summary.translation[0], 0.0f), "skybox x translation mismatch", failures);
        Assert(ApproximatelyEqual(summary.translation[1], 1.6f), "skybox y translation mismatch", failures);
        Assert(ApproximatelyEqual(summary.translation[2], 10.0f), "skybox z translation mismatch", failures);
        Assert(!skybox->indices.empty(), "skybox indices should not be empty", failures);
        if (!skybox->vertices.empty()) {
            ExpectColorNear(skybox->vertices.front(), skyboxColor, "skybox vertex color", failures);
        }
    }
}
} // namespace

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
        sdl3cpp::services::impl::ShaderScriptService shaderService(engineService, configService, logger);

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
