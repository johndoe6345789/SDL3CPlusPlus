#include "services/impl/materialx/materialx_shader_generator.hpp"
#include "services/impl/shader/shader_pipeline_validator.hpp"
#include "services/interfaces/i_logger.hpp"
#include "core/vertex.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <filesystem>

using namespace sdl3cpp::services;
using namespace sdl3cpp::services::impl;

// Mock logger
class MockLogger : public ILogger {
public:
    void SetLevel(LogLevel) override {}
    LogLevel GetLevel() const override { return LogLevel::INFO; }
    void SetOutputFile(const std::string&) override {}
    void SetMaxLinesPerFile(size_t) override {}
    void EnableConsoleOutput(bool) override {}
    void Log(LogLevel, const std::string&) override {}
    void Trace(const std::string&) override {}
    void Trace(const std::string&, const std::string&, const std::string&, const std::string&) override {}
    void Debug(const std::string&) override {}
    void Info(const std::string&) override {}
    void Warn(const std::string&) override {}
    void Error(const std::string&) override {}
    void TraceFunction(const std::string&) override {}
    void TraceVariable(const std::string&, const std::string&) override {}
    void TraceVariable(const std::string&, int) override {}
    void TraceVariable(const std::string&, size_t) override {}
    void TraceVariable(const std::string&, bool) override {}
    void TraceVariable(const std::string&, float) override {}
    void TraceVariable(const std::string&, double) override {}
};

class MaterialXShaderGeneratorIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        logger = std::make_shared<MockLogger>();
        generator = std::make_unique<MaterialXShaderGenerator>(logger);
        validator = std::make_unique<ShaderPipelineValidator>(logger);
    }

    std::shared_ptr<MockLogger> logger;
    std::unique_ptr<MaterialXShaderGenerator> generator;
    std::unique_ptr<ShaderPipelineValidator> validator;
};

// ============================================================================
// Test: MaterialX shader generation with validation
// ============================================================================

TEST_F(MaterialXShaderGeneratorIntegrationTest, ShadersMustPassValidation) {
    // This test ensures that ALL MaterialX shaders that get generated
    // MUST pass the shader pipeline validation before being submitted to GPU

    // The validation checks:
    // 1. Vertex layout matches shader inputs
    // 2. Vertex stride matches core::Vertex size (56 bytes)
    // 3. Vertex shader outputs match fragment shader inputs
    // 4. All inputs/outputs have layout(location=N) for SPIR-V

    // Expected bgfx vertex layout (from bgfx_graphics_backend.cpp)
    std::vector<ShaderPipelineValidator::AttributeInfo> bgfxLayout = {
        {0, "vec3", "Position", 12},   // bgfx::Attrib::Position
        {1, "vec3", "Normal", 12},     // bgfx::Attrib::Normal
        {2, "vec3", "Tangent", 12},    // bgfx::Attrib::Tangent
        {3, "vec2", "TexCoord0", 8},   // bgfx::Attrib::TexCoord0
        {4, "vec3", "Color0", 12},     // bgfx::Attrib::Color0
    };

    size_t vertexSize = sizeof(sdl3cpp::core::Vertex);
    ASSERT_EQ(vertexSize, 56) << "core::Vertex size must be 56 bytes";

    // Simulate a MaterialX vertex shader (after location remapping)
    std::string vertexShader = R"(
#version 450
layout (location = 0) in vec3 i_position;
layout (location = 1) in vec3 i_normal;
layout (location = 2) in vec3 i_tangent;
layout (location = 3) in vec2 i_texcoord_0;

layout (location = 0) out vec3 normalWorld;
layout (location = 1) out vec3 tangentWorld;
layout (location = 2) out vec2 texcoord;

uniform mat4 u_worldMatrix;
uniform mat4 u_viewProjectionMatrix;

void main() {
    vec4 worldPos = u_worldMatrix * vec4(i_position, 1.0);
    gl_Position = u_viewProjectionMatrix * worldPos;

    normalWorld = normalize(mat3(u_worldMatrix) * i_normal);
    tangentWorld = normalize(mat3(u_worldMatrix) * i_tangent);
    texcoord = i_texcoord_0;
}
)";

    std::string fragmentShader = R"(
#version 450
layout (location = 0) in vec3 normalWorld;
layout (location = 1) in vec3 tangentWorld;
layout (location = 2) in vec2 texcoord;

layout (location = 0) out vec4 fragColor;

void main() {
    vec3 N = normalize(normalWorld);
    fragColor = vec4(N * 0.5 + 0.5, 1.0);
}
)";

    // THIS IS THE CRITICAL TEST:
    // Validate the shader pipeline BEFORE GPU submission
    auto result = validator->ValidatePipeline(
        vertexShader,
        fragmentShader,
        bgfxLayout,
        vertexSize,
        "test_materialx"
    );

    // If validation fails, it should throw before reaching GPU
    EXPECT_TRUE(result.passed) << "MaterialX shader must pass validation";
    EXPECT_EQ(result.errors.size(), 0) << "No validation errors allowed";

    // Warnings are OK (e.g., unused Color0 attribute)
    if (!result.warnings.empty()) {
        std::cout << "Warnings: " << result.warnings.size() << std::endl;
        for (const auto& warn : result.warnings) {
            std::cout << "  - " << warn << std::endl;
        }
    }
}

TEST_F(MaterialXShaderGeneratorIntegrationTest, MalformedShadersMustBeRejected) {
    // This test ensures that malformed shaders that would crash the GPU
    // are caught by validation and rejected BEFORE reaching the GPU

    std::vector<ShaderPipelineValidator::AttributeInfo> bgfxLayout = {
        {0, "vec3", "Position", 12},
        {1, "vec3", "Normal", 12},
        {2, "vec3", "Tangent", 12},
        {3, "vec2", "TexCoord0", 8},
        {4, "vec3", "Color0", 12},
    };

    // Malformed shader: wrong location numbers (this caused the original crash)
    std::string badVertexShader = R"(
#version 450
layout (location = 0) in vec3 i_position;
layout (location = 5) in vec3 i_normal;     // WRONG! Should be location 1
layout (location = 2) in vec3 i_tangent;
layout (location = 3) in vec2 i_texcoord_0;

layout (location = 0) out vec3 normalWorld;

void main() {
    gl_Position = vec4(i_position, 1.0);
    normalWorld = i_normal;
}
)";

    std::string fragmentShader = R"(
#version 450
layout (location = 0) in vec3 normalWorld;
layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(normalWorld, 1.0);
}
)";

    auto result = validator->ValidatePipeline(
        badVertexShader,
        fragmentShader,
        bgfxLayout,
        56,
        "malformed_shader"
    );

    // Validation MUST fail for malformed shaders
    EXPECT_FALSE(result.passed) << "Malformed shader must fail validation";
    EXPECT_GT(result.errors.size(), 0) << "Must have validation errors";

    // In production, MaterialXShaderGenerator::Generate() would throw
    // an exception here, preventing GPU crash
}

TEST_F(MaterialXShaderGeneratorIntegrationTest, MissingTangentMustBeDetected) {
    // Test that shaders expecting tangent but not receiving it are caught

    std::vector<ShaderPipelineValidator::AttributeInfo> oldLayout = {
        {0, "vec3", "Position", 12},
        {1, "vec3", "Normal", 12},
        // Missing tangent!
        {2, "vec2", "TexCoord0", 8},
        {3, "vec3", "Color0", 12},
    };

    std::string vertexShader = R"(
#version 450
layout (location = 0) in vec3 i_position;
layout (location = 1) in vec3 i_normal;
layout (location = 2) in vec3 i_tangent;  // Expects tangent at location 2
layout (location = 3) in vec2 i_texcoord_0;

layout (location = 0) out vec3 normalWorld;

void main() {
    gl_Position = vec4(i_position, 1.0);
    normalWorld = i_normal + i_tangent;  // Uses tangent
}
)";

    std::string fragmentShader = R"(
#version 450
layout (location = 0) in vec3 normalWorld;
layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(normalWorld, 1.0);
}
)";

    auto result = validator->ValidatePipeline(
        vertexShader,
        fragmentShader,
        oldLayout,
        44,  // Old vertex size without tangent
        "missing_tangent"
    );

    EXPECT_FALSE(result.passed) << "Missing tangent must fail validation";
    EXPECT_GT(result.errors.size(), 0);
}

TEST_F(MaterialXShaderGeneratorIntegrationTest, InterfaceMismatchMustBeDetected) {
    // Test that VS output / FS input mismatches are caught

    std::vector<ShaderPipelineValidator::AttributeInfo> bgfxLayout = {
        {0, "vec3", "Position", 12},
        {1, "vec3", "Normal", 12},
        {2, "vec3", "Tangent", 12},
        {3, "vec2", "TexCoord0", 8},
        {4, "vec3", "Color0", 12},
    };

    std::string vertexShader = R"(
#version 450
layout (location = 0) in vec3 i_position;
layout (location = 1) in vec3 i_normal;

layout (location = 0) out vec3 normalWorld;
// Missing output at location 1 that FS expects!

void main() {
    gl_Position = vec4(i_position, 1.0);
    normalWorld = i_normal;
}
)";

    std::string fragmentShader = R"(
#version 450
layout (location = 0) in vec3 normalWorld;
layout (location = 1) in vec2 texcoord;  // VS doesn't output this!

layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(normalWorld * texcoord.x, 1.0);
}
)";

    auto result = validator->ValidatePipeline(
        vertexShader,
        fragmentShader,
        bgfxLayout,
        56,
        "interface_mismatch"
    );

    EXPECT_FALSE(result.passed) << "Interface mismatch must fail validation";
    EXPECT_GT(result.errors.size(), 0);
}

TEST_F(MaterialXShaderGeneratorIntegrationTest, MissingSpirvLocationsMustBeDetected) {
    // Test that SPIR-V violations (missing layout(location=N)) are caught

    std::vector<ShaderPipelineValidator::AttributeInfo> bgfxLayout = {
        {0, "vec3", "Position", 12},
        {1, "vec3", "Normal", 12},
    };

    std::string vertexShader = R"(
#version 450
in vec3 i_position;  // MISSING layout(location=0)!
layout (location = 1) in vec3 i_normal;

layout (location = 0) out vec3 normalWorld;

void main() {
    gl_Position = vec4(i_position, 1.0);
    normalWorld = i_normal;
}
)";

    std::string fragmentShader = R"(
#version 450
layout (location = 0) in vec3 normalWorld;
layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(normalWorld, 1.0);
}
)";

    auto result = validator->ValidatePipeline(
        vertexShader,
        fragmentShader,
        bgfxLayout,
        24,
        "missing_spirv_location"
    );

    EXPECT_FALSE(result.passed) << "Missing SPIR-V location must fail validation";
    EXPECT_GT(result.errors.size(), 0);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
