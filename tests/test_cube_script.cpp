#include "script/cube_script.hpp"

#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
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

void Assert(bool condition, const std::string& message, int& failures) {
    if (!condition) {
        std::cerr << "test failure: " << message << '\n';
        ++failures;
    }
}
} // namespace

int main() {
    int failures = 0;
    auto scriptPath = GetTestScriptPath();
    std::cout << "Loading Lua fixture: " << scriptPath << '\n';

    try {
        sdl3cpp::script::CubeScript cubeScript(scriptPath);
        auto objects = cubeScript.LoadSceneObjects();
        Assert(objects.size() == 1, "expected exactly one scene object", failures);
        if (!objects.empty()) {
            const auto& object = objects.front();
            Assert(object.vertices.size() == 3, "scene object should yield three vertices", failures);
            Assert(object.indices.size() == 3, "scene object should yield three indices", failures);
            Assert(object.shaderKey == "test", "shader key should match fixture", failures);
            const std::vector<uint16_t> expectedIndices{0, 1, 2};
            Assert(object.indices == expectedIndices, "indices should be zero-based", failures);
            Assert(object.computeModelMatrixRef != LUA_REFNIL,
                   "vertex object must keep a Lua reference", failures);

            auto objectMatrix = cubeScript.ComputeModelMatrix(object.computeModelMatrixRef, 0.5f);
            ExpectIdentity(objectMatrix, "object compute_model_matrix", failures);
        }

        auto fallbackMatrix = cubeScript.ComputeModelMatrix(LUA_REFNIL, 1.0f);
        ExpectIdentity(fallbackMatrix, "global compute_model_matrix", failures);

        auto viewProjection = cubeScript.GetViewProjectionMatrix(1.33f);
        ExpectIdentity(viewProjection, "view_projection matrix", failures);

        auto shaderMap = cubeScript.LoadShaderPathsMap();
        Assert(shaderMap.size() == 1, "expected a single shader variant", failures);
        auto testEntry = shaderMap.find("test");
        Assert(testEntry != shaderMap.end(), "shader map missing "
               "test entry", failures);
        if (testEntry != shaderMap.end()) {
            Assert(testEntry->second.vertex == "shaders/test.vert.spv", "vertex shader path", failures);
            Assert(testEntry->second.fragment == "shaders/test.frag.spv", "fragment shader path", failures);
        }
    } catch (const std::exception& ex) {
        std::cerr << "exception during tests: " << ex.what() << '\n';
        return 1;
    }

    if (failures == 0) {
        std::cout << "cube_script_tests: PASSED\n";
    } else {
        std::cerr << "cube_script_tests: FAILED (" << failures << " errors)\n";
    }

    return failures;
}
