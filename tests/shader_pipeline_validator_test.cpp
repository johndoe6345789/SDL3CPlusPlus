#include "services/impl/shader/shader_pipeline_validator.hpp"
#include "services/interfaces/i_logger.hpp"
#include "core/vertex.hpp"
#include <gtest/gtest.h>
#include <memory>

using namespace sdl3cpp::services;

// Mock logger for testing
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

class ShaderPipelineValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        logger = std::make_shared<MockLogger>();
        validator = std::make_unique<ShaderPipelineValidator>(logger);
    }

    std::shared_ptr<MockLogger> logger;
    std::unique_ptr<ShaderPipelineValidator> validator;
};

// ============================================================================
// Test: Extract Shader Inputs
// ============================================================================

TEST_F(ShaderPipelineValidatorTest, ExtractShaderInputs_ValidGLSL) {
    std::string glsl = R"(
#version 450
layout (location = 0) in vec3 i_position;
layout (location = 1) in vec3 i_normal;
layout (location = 2) in vec3 i_tangent;
layout (location = 3) in vec2 i_texcoord_0;

void main() {
    gl_Position = vec4(i_position, 1.0);
}
)";

    auto inputs = validator->ExtractShaderInputs(glsl);

    ASSERT_EQ(inputs.size(), 4);

    EXPECT_EQ(inputs[0].location, 0);
    EXPECT_EQ(inputs[0].type, "vec3");
    EXPECT_EQ(inputs[0].name, "i_position");
    EXPECT_EQ(inputs[0].sizeBytes, 12);

    EXPECT_EQ(inputs[1].location, 1);
    EXPECT_EQ(inputs[1].type, "vec3");
    EXPECT_EQ(inputs[1].name, "i_normal");

    EXPECT_EQ(inputs[2].location, 2);
    EXPECT_EQ(inputs[2].type, "vec3");
    EXPECT_EQ(inputs[2].name, "i_tangent");

    EXPECT_EQ(inputs[3].location, 3);
    EXPECT_EQ(inputs[3].type, "vec2");
    EXPECT_EQ(inputs[3].name, "i_texcoord_0");
    EXPECT_EQ(inputs[3].sizeBytes, 8);
}

TEST_F(ShaderPipelineValidatorTest, ExtractShaderInputs_CompactLayout) {
    std::string glsl = R"(
layout(location=0)in vec3 pos;
layout(location=1)in vec2 uv;
)";

    auto inputs = validator->ExtractShaderInputs(glsl);

    ASSERT_EQ(inputs.size(), 2);
    EXPECT_EQ(inputs[0].location, 0);
    EXPECT_EQ(inputs[1].location, 1);
}

TEST_F(ShaderPipelineValidatorTest, ExtractShaderInputs_NoInputs) {
    std::string glsl = R"(
#version 450
void main() {
    gl_Position = vec4(0.0);
}
)";

    auto inputs = validator->ExtractShaderInputs(glsl);
    EXPECT_EQ(inputs.size(), 0);
}

// ============================================================================
// Test: Extract Shader Outputs
// ============================================================================

TEST_F(ShaderPipelineValidatorTest, ExtractShaderOutputs_Valid) {
    std::string glsl = R"(
#version 450
layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec2 v_texcoord;

void main() {
    v_normal = vec3(0.0);
    v_texcoord = vec2(0.0);
}
)";

    auto outputs = validator->ExtractShaderOutputs(glsl);

    ASSERT_EQ(outputs.size(), 2);
    EXPECT_EQ(outputs[0].location, 0);
    EXPECT_EQ(outputs[0].name, "v_normal");
    EXPECT_EQ(outputs[1].location, 1);
    EXPECT_EQ(outputs[1].name, "v_texcoord");
}

// ============================================================================
// Test: Validate Vertex Layout Match
// ============================================================================

TEST_F(ShaderPipelineValidatorTest, ValidateVertexLayoutMatch_Perfect) {
    std::vector<ShaderPipelineValidator::AttributeInfo> shaderInputs = {
        {0, "vec3", "i_position", 12},
        {1, "vec3", "i_normal", 12},
        {2, "vec2", "i_texcoord", 8},
    };

    std::vector<ShaderPipelineValidator::AttributeInfo> layoutAttribs = {
        {0, "vec3", "Position", 12},
        {1, "vec3", "Normal", 12},
        {2, "vec2", "TexCoord", 8},
    };

    auto result = validator->ValidateVertexLayoutMatch(shaderInputs, layoutAttribs, "test_shader");

    EXPECT_TRUE(result.passed);
    EXPECT_EQ(result.errors.size(), 0);
}

TEST_F(ShaderPipelineValidatorTest, ValidateVertexLayoutMatch_MissingAttribute) {
    std::vector<ShaderPipelineValidator::AttributeInfo> shaderInputs = {
        {0, "vec3", "i_position", 12},
        {1, "vec3", "i_normal", 12},
        {2, "vec3", "i_tangent", 12},  // Shader expects tangent
    };

    std::vector<ShaderPipelineValidator::AttributeInfo> layoutAttribs = {
        {0, "vec3", "Position", 12},
        {1, "vec3", "Normal", 12},
        // Missing tangent at location 2!
    };

    auto result = validator->ValidateVertexLayoutMatch(shaderInputs, layoutAttribs, "test_shader");

    EXPECT_FALSE(result.passed);
    EXPECT_GT(result.errors.size(), 0);
    EXPECT_TRUE(result.errors[0].find("location 2") != std::string::npos);
    EXPECT_TRUE(result.errors[0].find("i_tangent") != std::string::npos);
}

TEST_F(ShaderPipelineValidatorTest, ValidateVertexLayoutMatch_TypeMismatch) {
    std::vector<ShaderPipelineValidator::AttributeInfo> shaderInputs = {
        {0, "vec3", "i_position", 12},
        {1, "vec4", "i_normal", 16},  // Expects vec4
    };

    std::vector<ShaderPipelineValidator::AttributeInfo> layoutAttribs = {
        {0, "vec3", "Position", 12},
        {1, "vec3", "Normal", 12},    // Provides vec3
    };

    auto result = validator->ValidateVertexLayoutMatch(shaderInputs, layoutAttribs, "test_shader");

    EXPECT_FALSE(result.passed);
    EXPECT_GT(result.errors.size(), 0);
    EXPECT_TRUE(result.errors[0].find("Type mismatch") != std::string::npos);
}

TEST_F(ShaderPipelineValidatorTest, ValidateVertexLayoutMatch_ShuffledLocations) {
    std::vector<ShaderPipelineValidator::AttributeInfo> shaderInputs = {
        {0, "vec3", "i_position", 12},
        {1, "vec2", "i_texcoord_0", 8},  // Wrong location/type for bgfx layout.
        {2, "vec3", "i_tangent", 12},
        {3, "vec3", "i_normal", 12},    // Normal moved to location 3.
    };

    std::vector<ShaderPipelineValidator::AttributeInfo> layoutAttribs = {
        {0, "vec3", "Position", 12},
        {1, "vec3", "Normal", 12},
        {2, "vec3", "Tangent", 12},
        {3, "vec2", "TexCoord0", 8},
    };

    auto result = validator->ValidateVertexLayoutMatch(shaderInputs, layoutAttribs, "test_shader");

    EXPECT_FALSE(result.passed);
    EXPECT_GT(result.errors.size(), 0);
    EXPECT_TRUE(result.errors[0].find("location 1") != std::string::npos);
}

TEST_F(ShaderPipelineValidatorTest, ValidateVertexLayoutMatch_UnusedAttribute) {
    std::vector<ShaderPipelineValidator::AttributeInfo> shaderInputs = {
        {0, "vec3", "i_position", 12},
    };

    std::vector<ShaderPipelineValidator::AttributeInfo> layoutAttribs = {
        {0, "vec3", "Position", 12},
        {1, "vec3", "Normal", 12},    // Layout provides but shader doesn't use
        {2, "vec2", "TexCoord", 8},
    };

    auto result = validator->ValidateVertexLayoutMatch(shaderInputs, layoutAttribs, "test_shader");

    EXPECT_TRUE(result.passed);  // Not an error, just warning
    EXPECT_EQ(result.warnings.size(), 2);  // 2 unused attributes
}

// ============================================================================
// Test: Validate Vertex Stride
// ============================================================================

TEST_F(ShaderPipelineValidatorTest, ValidateVertexStride_Correct) {
    std::vector<ShaderPipelineValidator::AttributeInfo> layoutAttribs = {
        {0, "vec3", "Position", 12},
        {1, "vec3", "Normal", 12},
        {2, "vec3", "Tangent", 12},
        {3, "vec2", "TexCoord", 8},
        {4, "vec3", "Color", 12},
    };

    size_t actualSize = sizeof(sdl3cpp::core::Vertex);  // Should be 56

    auto result = validator->ValidateVertexStride(layoutAttribs, actualSize);

    EXPECT_TRUE(result.passed);
    EXPECT_EQ(result.errors.size(), 0);
}

TEST_F(ShaderPipelineValidatorTest, ValidateVertexStride_Mismatch) {
    std::vector<ShaderPipelineValidator::AttributeInfo> layoutAttribs = {
        {0, "vec3", "Position", 12},
        {1, "vec3", "Normal", 12},
        {2, "vec2", "TexCoord", 8},
    };

    size_t actualSize = 56;  // Actual struct is 56 but layout expects 32

    auto result = validator->ValidateVertexStride(layoutAttribs, actualSize);

    EXPECT_FALSE(result.passed);
    EXPECT_GT(result.errors.size(), 0);
    EXPECT_TRUE(result.errors[0].find("stride mismatch") != std::string::npos);
}

// ============================================================================
// Test: Validate Interface Matching
// ============================================================================

TEST_F(ShaderPipelineValidatorTest, ValidateInterfaceMatching_Perfect) {
    std::vector<ShaderPipelineValidator::AttributeInfo> vsOutputs = {
        {0, "vec3", "v_normal", 12},
        {1, "vec2", "v_texcoord", 8},
    };

    std::vector<ShaderPipelineValidator::AttributeInfo> fsInputs = {
        {0, "vec3", "v_normal", 12},
        {1, "vec2", "v_texcoord", 8},
    };

    auto result = validator->ValidateInterfaceMatching(vsOutputs, fsInputs, "vs", "fs");

    EXPECT_TRUE(result.passed);
    EXPECT_EQ(result.errors.size(), 0);
}

TEST_F(ShaderPipelineValidatorTest, ValidateInterfaceMatching_MissingOutput) {
    std::vector<ShaderPipelineValidator::AttributeInfo> vsOutputs = {
        {0, "vec3", "v_normal", 12},
        // Missing location 1!
    };

    std::vector<ShaderPipelineValidator::AttributeInfo> fsInputs = {
        {0, "vec3", "v_normal", 12},
        {1, "vec2", "v_texcoord", 8},  // Fragment expects this
    };

    auto result = validator->ValidateInterfaceMatching(vsOutputs, fsInputs, "vs", "fs");

    EXPECT_FALSE(result.passed);
    EXPECT_GT(result.errors.size(), 0);
    EXPECT_TRUE(result.errors[0].find("location 1") != std::string::npos);
}

TEST_F(ShaderPipelineValidatorTest, ValidateInterfaceMatching_TypeMismatch) {
    std::vector<ShaderPipelineValidator::AttributeInfo> vsOutputs = {
        {0, "vec3", "v_data", 12},
    };

    std::vector<ShaderPipelineValidator::AttributeInfo> fsInputs = {
        {0, "vec4", "v_data", 16},  // Different type!
    };

    auto result = validator->ValidateInterfaceMatching(vsOutputs, fsInputs, "vs", "fs");

    EXPECT_FALSE(result.passed);
    EXPECT_GT(result.errors.size(), 0);
    EXPECT_TRUE(result.errors[0].find("Type mismatch") != std::string::npos);
}

// ============================================================================
// Test: Validate SPIR-V Requirements
// ============================================================================

TEST_F(ShaderPipelineValidatorTest, ValidateSpirvRequirements_AllHaveLocation) {
    std::string glsl = R"(
#version 450
layout (location = 0) in vec3 i_position;
layout (location = 1) in vec3 i_normal;
layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(i_position + i_normal, 1.0);
}
)";

    auto result = validator->ValidateSpirvRequirements(glsl, "test_shader");

    EXPECT_TRUE(result.passed);
    EXPECT_EQ(result.errors.size(), 0);
}

TEST_F(ShaderPipelineValidatorTest, ValidateSpirvRequirements_MissingLocation) {
    std::string glsl = R"(
#version 450
in vec3 i_position;  // Missing layout(location=N)!
layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(i_position, 1.0);
}
)";

    auto result = validator->ValidateSpirvRequirements(glsl, "test_shader");

    EXPECT_FALSE(result.passed);
    EXPECT_GT(result.errors.size(), 0);
    EXPECT_TRUE(result.errors[0].find("SPIR-V requires") != std::string::npos);
}

TEST_F(ShaderPipelineValidatorTest, ValidateSpirvRequirements_BuiltinsIgnored) {
    std::string glsl = R"(
#version 450
layout (location = 0) in vec3 i_position;

void main() {
    gl_Position = vec4(i_position, 1.0);  // gl_ built-in should be ignored
}
)";

    auto result = validator->ValidateSpirvRequirements(glsl, "test_shader");

    EXPECT_TRUE(result.passed);
}

// ============================================================================
// Test: Complete Pipeline Validation
// ============================================================================

TEST_F(ShaderPipelineValidatorTest, ValidatePipeline_FullSuccess) {
    std::string vertexShader = R"(
#version 450
layout (location = 0) in vec3 i_position;
layout (location = 1) in vec3 i_normal;
layout (location = 2) in vec3 i_tangent;
layout (location = 3) in vec2 i_texcoord_0;

layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec2 v_texcoord;

void main() {
    gl_Position = vec4(i_position, 1.0);
    v_normal = i_normal;
    v_texcoord = i_texcoord_0;
}
)";

    std::string fragmentShader = R"(
#version 450
layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec2 v_texcoord;

layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(v_normal * 0.5 + 0.5, 1.0);
}
)";

    std::vector<ShaderPipelineValidator::AttributeInfo> layoutAttribs = {
        {0, "vec3", "Position", 12},
        {1, "vec3", "Normal", 12},
        {2, "vec3", "Tangent", 12},
        {3, "vec2", "TexCoord0", 8},
        {4, "vec3", "Color0", 12},
    };

    auto result = validator->ValidatePipeline(
        vertexShader, fragmentShader, layoutAttribs,
        sizeof(sdl3cpp::core::Vertex), "test_pipeline"
    );

    EXPECT_TRUE(result.passed);
    EXPECT_EQ(result.errors.size(), 0);
}

TEST_F(ShaderPipelineValidatorTest, ValidatePipeline_MultipleErrors) {
    std::string vertexShader = R"(
#version 450
layout (location = 0) in vec3 i_position;
layout (location = 5) in vec3 i_invalid;  // Wrong location!

layout (location = 0) out vec3 v_normal;

void main() {
    gl_Position = vec4(i_position, 1.0);
    v_normal = vec3(0.0);
}
)";

    std::string fragmentShader = R"(
#version 450
layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec2 v_missing;  // VS doesn't output this!

layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(v_normal, 1.0);
}
)";

    std::vector<ShaderPipelineValidator::AttributeInfo> layoutAttribs = {
        {0, "vec3", "Position", 12},
        {1, "vec3", "Normal", 12},
    };

    auto result = validator->ValidatePipeline(
        vertexShader, fragmentShader, layoutAttribs,
        32,  // Wrong size! Should be 56
        "test_pipeline"
    );

    EXPECT_FALSE(result.passed);
    EXPECT_GT(result.errors.size(), 2);  // Should have multiple errors
}

// ============================================================================
// Test: Real-World MaterialX Scenario
// ============================================================================

TEST_F(ShaderPipelineValidatorTest, RealWorld_MaterialXShader) {
    // This is the actual scenario that was causing crashes
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

    // This is the expected bgfx layout
    std::vector<ShaderPipelineValidator::AttributeInfo> bgfxLayout = {
        {0, "vec3", "Position", 12},
        {1, "vec3", "Normal", 12},
        {2, "vec3", "Tangent", 12},
        {3, "vec2", "TexCoord0", 8},
        {4, "vec3", "Color0", 12},
    };

    auto result = validator->ValidatePipeline(
        vertexShader, fragmentShader, bgfxLayout,
        sizeof(sdl3cpp::core::Vertex),
        "materialx_floor"
    );

    // This should PASS with the fix
    EXPECT_TRUE(result.passed) << "MaterialX validation should pass after location remapping fix";
    EXPECT_EQ(result.errors.size(), 0);
}

// ============================================================================
// Test: Edge Cases
// ============================================================================

TEST_F(ShaderPipelineValidatorTest, EdgeCase_EmptyShaders) {
    auto result = validator->ValidatePipeline("", "", {}, 0, "empty");

    EXPECT_TRUE(result.passed);  // Empty is technically valid
}

TEST_F(ShaderPipelineValidatorTest, EdgeCase_CommentsAndWhitespace) {
    std::string glsl = R"(
// Comment line
    /* Block comment */
    layout (location = 0) in vec3 i_position;  // Inline comment

    // Another comment
)";

    auto inputs = validator->ExtractShaderInputs(glsl);
    EXPECT_EQ(inputs.size(), 1);
}

TEST_F(ShaderPipelineValidatorTest, EdgeCase_NonContinuousLocations) {
    std::vector<ShaderPipelineValidator::AttributeInfo> shaderInputs = {
        {0, "vec3", "i_position", 12},
        {2, "vec3", "i_normal", 12},     // Gap at location 1!
        {5, "vec2", "i_texcoord", 8},    // Gap at 3, 4!
    };

    std::vector<ShaderPipelineValidator::AttributeInfo> layoutAttribs = {
        {0, "vec3", "Position", 12},
        {2, "vec3", "Normal", 12},
        {5, "vec2", "TexCoord", 8},
    };

    auto result = validator->ValidateVertexLayoutMatch(shaderInputs, layoutAttribs, "test");

    EXPECT_TRUE(result.passed);  // Gaps are allowed, just inefficient
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
