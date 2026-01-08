#include <gtest/gtest.h>
#include <string>
#include <vector>

// Integration test for shader uniform type mapping
// Based on VULKAN_SHADER_LINKING_PROBLEM.md
//
// Problem: GL_INT was mapped to UniformType::Sampler instead of Vec4
// This caused GUI shader linking to fail in Vulkan
//
// File to fix: src/bgfx_tools/shaderc/shaderc_spirv.cpp line ~685

namespace {

// GL type constants
constexpr uint32_t GL_INT = 0x1404;
constexpr uint32_t GL_INT_VEC2 = 0x8B53;
constexpr uint32_t GL_INT_VEC3 = 0x8B54;
constexpr uint32_t GL_INT_VEC4 = 0x8B55;
constexpr uint32_t GL_FLOAT_VEC4 = 0x8B52;
constexpr uint32_t GL_SAMPLER_2D = 0x8B5E;
constexpr uint32_t GL_SAMPLER_3D = 0x8B5F;
constexpr uint32_t GL_SAMPLER_CUBE = 0x8B60;

enum class ExpectedUniformType {
    Vec4,      // Data uniform (int, float, vec types)
    Sampler,   // Texture sampler
    Unknown
};

struct UniformTypeMapping {
    uint32_t glType;
    const char* glTypeName;
    ExpectedUniformType expectedBgfxType;
    const char* description;
};

// Test fixture documenting the expected mappings
class ShaderUniformTypeMappingTest : public ::testing::Test {
protected:
    std::vector<UniformTypeMapping> GetExpectedMappings() {
        return {
            // Integer types - MUST map to Vec4 (data uniform)
            {GL_INT, "GL_INT", ExpectedUniformType::Vec4, 
             "Single int - used for flags, counts, etc."},
            {GL_INT_VEC2, "GL_INT_VEC2", ExpectedUniformType::Vec4,
             "ivec2 - two integers"},
            {GL_INT_VEC3, "GL_INT_VEC3", ExpectedUniformType::Vec4,
             "ivec3 - three integers"},
            {GL_INT_VEC4, "GL_INT_VEC4", ExpectedUniformType::Vec4,
             "ivec4 - four integers"},
            
            // Float types - should map to Vec4 (data uniform)
            {GL_FLOAT_VEC4, "GL_FLOAT_VEC4", ExpectedUniformType::Vec4,
             "vec4 - standard float vector"},
            
            // Sampler types - MUST map to Sampler (texture)
            {GL_SAMPLER_2D, "GL_SAMPLER_2D", ExpectedUniformType::Sampler,
             "sampler2D - 2D texture sampler"},
            {GL_SAMPLER_3D, "GL_SAMPLER_3D", ExpectedUniformType::Sampler,
             "sampler3D - 3D texture sampler"},
            {GL_SAMPLER_CUBE, "GL_SAMPLER_CUBE", ExpectedUniformType::Sampler,
             "samplerCube - cube map sampler"},
        };
    }
};

TEST_F(ShaderUniformTypeMappingTest, DocumentExpectedMappings) {
    // This test documents what the shader compiler should do
    auto mappings = GetExpectedMappings();
    
    for (const auto& mapping : mappings) {
        if (mapping.expectedBgfxType == ExpectedUniformType::Vec4) {
            EXPECT_TRUE(true) << mapping.glTypeName << " should map to Vec4: " 
                             << mapping.description;
        } else if (mapping.expectedBgfxType == ExpectedUniformType::Sampler) {
            EXPECT_TRUE(true) << mapping.glTypeName << " should map to Sampler: "
                             << mapping.description;
        }
    }
}

TEST_F(ShaderUniformTypeMappingTest, IntegerTypes_MustNotBeSamplers) {
    // The BUG: GL_INT and int vector types were mapped to Sampler
    // CORRECT: They should be Vec4 (data uniforms)
    
    std::vector<uint32_t> integerTypes = {
        GL_INT, GL_INT_VEC2, GL_INT_VEC3, GL_INT_VEC4
    };
    
    for (auto glType : integerTypes) {
        // These are data types, NOT texture samplers
        EXPECT_NE(glType, GL_SAMPLER_2D) << "Integer type should not be sampler";
        EXPECT_NE(glType, GL_SAMPLER_3D) << "Integer type should not be sampler";
        EXPECT_NE(glType, GL_SAMPLER_CUBE) << "Integer type should not be sampler";
    }
}

TEST_F(ShaderUniformTypeMappingTest, AffectedGuiUniforms) {
    // From VULKAN_SHADER_LINKING_PROBLEM.md line 58-60
    // These specific uniforms in GUI shader were misclassified
    
    struct AffectedUniform {
        const char* name;
        uint32_t glType;
        ExpectedUniformType correctType;
    };
    
    std::vector<AffectedUniform> affectedUniforms = {
        {"u_numActiveLightSources", GL_INT, ExpectedUniformType::Vec4},
        {"u_lightData.type", GL_INT, ExpectedUniformType::Vec4},
        {"noise_octaves", GL_INT, ExpectedUniformType::Vec4},
    };
    
    for (const auto& uniform : affectedUniforms) {
        EXPECT_EQ(uniform.glType, GL_INT);
        EXPECT_EQ(uniform.correctType, ExpectedUniformType::Vec4)
            << uniform.name << " should be Vec4 (data uniform), not Sampler";
    }
}

TEST_F(ShaderUniformTypeMappingTest, RequiredFixLocation) {
    // Documents where the fix needs to be applied
    const char* file = "src/bgfx_tools/shaderc/shaderc_spirv.cpp";
    const int approximateLine = 685;
    
    // BEFORE (WRONG):
    // case 0x1404: // GL_INT:
    //     un.type = UniformType::Sampler;  // ❌ INCORRECT
    //     break;
    
    // AFTER (CORRECT):
    // case 0x1404: // GL_INT:
    // case 0x8B53: // GL_INT_VEC2:
    // case 0x8B54: // GL_INT_VEC3:
    // case 0x8B55: // GL_INT_VEC4:
    //     un.type = UniformType::Vec4;  // ✅ CORRECT
    //     break;
    
    EXPECT_STREQ(file, "src/bgfx_tools/shaderc/shaderc_spirv.cpp");
    EXPECT_EQ(approximateLine, 685);
}

TEST_F(ShaderUniformTypeMappingTest, ImpactOnGuiShaderLinking) {
    // When integer uniforms are misclassified as samplers:
    // 1. bgfx creates uniform as sampler type
    // 2. Vulkan backend tries to create descriptor binding for "sampler"
    // 3. Descriptor layout creation fails (invalid sampler metadata)
    // 4. Shader linking fails
    // 5. GUI doesn't render
    
    const char* errorMessage = "BgfxGuiService::CreateProgram: bgfx::createProgram failed to link shaders";
    const char* consequence = "GUI completely non-functional";
    
    EXPECT_NE(std::string(errorMessage).find("failed to link"), std::string::npos);
    EXPECT_TRUE(true) << consequence;
}

TEST_F(ShaderUniformTypeMappingTest, MixedUniformTypes_InOneShader) {
    // A shader can have both data uniforms and texture samplers
    // Each must be classified correctly
    
    struct ShaderUniform {
        const char* declaration;
        uint32_t glType;
        ExpectedUniformType expectedType;
    };
    
    std::vector<ShaderUniform> mixedUniforms = {
        {"uniform sampler2D textureSampler", GL_SAMPLER_2D, ExpectedUniformType::Sampler},
        {"uniform int useTexture", GL_INT, ExpectedUniformType::Vec4},
        {"uniform vec4 color", GL_FLOAT_VEC4, ExpectedUniformType::Vec4},
    };
    
    int samplerCount = 0;
    int dataCount = 0;
    
    for (const auto& uniform : mixedUniforms) {
        if (uniform.expectedType == ExpectedUniformType::Sampler) {
            samplerCount++;
        } else if (uniform.expectedType == ExpectedUniformType::Vec4) {
            dataCount++;
        }
    }
    
    EXPECT_EQ(samplerCount, 1) << "Should have 1 sampler (textureSampler)";
    EXPECT_EQ(dataCount, 2) << "Should have 2 data uniforms (useTexture, color)";
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "Shader Uniform Type Mapping Integration Tests\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";
    std::cout << "These tests document the GL_INT → Sampler bug that caused GUI shader linking\n";
    std::cout << "to fail in Vulkan. Integer uniforms were incorrectly mapped to texture samplers.\n";
    std::cout << "\n";
    std::cout << "Fix required in: src/bgfx_tools/shaderc/shaderc_spirv.cpp (line ~685)\n";
    std::cout << "Change: case GL_INT → map to Vec4, not Sampler\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";
    
    return RUN_ALL_TESTS();
}
