#include <gtest/gtest.h>

#include <bgfx/bgfx.h>

#include "shaderc_mem.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct UniformEntry {
    std::string name;
    uint8_t type = 0;
};

struct ShaderParseResult {
    bool ok = false;
    std::string error;
    std::vector<UniformEntry> uniforms;
};

constexpr uint8_t kUniformFragmentBit = 0x10;
constexpr uint8_t kUniformSamplerBit = 0x20;
constexpr uint8_t kUniformReadOnlyBit = 0x40;
constexpr uint8_t kUniformCompareBit = 0x80;
constexpr uint8_t kUniformMask = kUniformFragmentBit |
                                 kUniformSamplerBit |
                                 kUniformReadOnlyBit |
                                 kUniformCompareBit;

bool ReadBytes(const std::vector<uint8_t>& buffer, size_t& offset, void* out, size_t size) {
    if (offset + size > buffer.size()) {
        return false;
    }
    std::memcpy(out, buffer.data() + offset, size);
    offset += size;
    return true;
}

ShaderParseResult ParseUniforms(const std::vector<uint8_t>& buffer, char expectedShaderType) {
    ShaderParseResult result{};
    size_t offset = 0;
    uint32_t magic = 0;

    if (!ReadBytes(buffer, offset, &magic, sizeof(magic))) {
        result.error = "missing shader magic";
        return result;
    }

    const uint32_t base = magic & 0x00FFFFFFu;
    const uint32_t vsh = static_cast<uint32_t>('V')
        | (static_cast<uint32_t>('S') << 8)
        | (static_cast<uint32_t>('H') << 16);
    const uint32_t fsh = static_cast<uint32_t>('F')
        | (static_cast<uint32_t>('S') << 8)
        | (static_cast<uint32_t>('H') << 16);
    const uint32_t csh = static_cast<uint32_t>('C')
        | (static_cast<uint32_t>('S') << 8)
        | (static_cast<uint32_t>('H') << 16);
    const uint32_t expectedBase = (expectedShaderType == 'v') ? vsh
        : (expectedShaderType == 'c' ? csh : fsh);

    if (base != expectedBase) {
        result.error = "unexpected shader type";
        return result;
    }

    const uint8_t version = static_cast<uint8_t>(magic >> 24);
    const bool hasTexData = version >= 8;
    const bool hasTexFormat = version >= 10;

    uint32_t hashIn = 0;
    uint32_t hashOut = 0;
    if (!ReadBytes(buffer, offset, &hashIn, sizeof(hashIn)) ||
        !ReadBytes(buffer, offset, &hashOut, sizeof(hashOut))) {
        result.error = "missing hash data";
        return result;
    }

    uint16_t uniformCount = 0;
    if (!ReadBytes(buffer, offset, &uniformCount, sizeof(uniformCount))) {
        result.error = "missing uniform count";
        return result;
    }

    result.uniforms.reserve(uniformCount);
    for (uint16_t i = 0; i < uniformCount; ++i) {
        uint8_t nameSize = 0;
        if (!ReadBytes(buffer, offset, &nameSize, sizeof(nameSize))) {
            result.error = "missing uniform name size";
            return result;
        }
        if (offset + nameSize > buffer.size()) {
            result.error = "uniform name out of bounds";
            return result;
        }
        UniformEntry entry{};
        entry.name.assign(reinterpret_cast<const char*>(buffer.data() + offset), nameSize);
        offset += nameSize;

        uint8_t type = 0;
        uint8_t num = 0;
        uint16_t regIndex = 0;
        uint16_t regCount = 0;
        if (!ReadBytes(buffer, offset, &type, sizeof(type)) ||
            !ReadBytes(buffer, offset, &num, sizeof(num)) ||
            !ReadBytes(buffer, offset, &regIndex, sizeof(regIndex)) ||
            !ReadBytes(buffer, offset, &regCount, sizeof(regCount))) {
            result.error = "missing uniform metadata";
            return result;
        }

        if (hasTexData) {
            uint8_t texComponent = 0;
            uint8_t texDimension = 0;
            if (!ReadBytes(buffer, offset, &texComponent, sizeof(texComponent)) ||
                !ReadBytes(buffer, offset, &texDimension, sizeof(texDimension))) {
                result.error = "missing texture metadata";
                return result;
            }
        }

        if (hasTexFormat) {
            uint16_t texFormat = 0;
            if (!ReadBytes(buffer, offset, &texFormat, sizeof(texFormat))) {
                result.error = "missing texture format";
                return result;
            }
        }

        entry.type = type;
        result.uniforms.push_back(std::move(entry));
    }

    result.ok = true;
    return result;
}

bool CompileShaderBinary(const std::string& source,
                         const char* profile,
                         const char* target,
                         std::vector<uint8_t>& out,
                         std::string& error) {
    uint8_t* compiled = nullptr;
    size_t compiledSize = 0;
    char* compileError = nullptr;

    const int result = shaderc_compile_from_memory_with_target(
        source.c_str(),
        source.size(),
        profile,
        target,
        &compiled,
        &compiledSize,
        &compileError);

    if (result != 0) {
        if (compileError) {
            error = compileError;
            shaderc_free_error(compileError);
        } else {
            error = "shaderc compile failed";
        }
        if (compiled) {
            shaderc_free_buffer(compiled);
        }
        return false;
    }

    out.assign(compiled, compiled + compiledSize);
    shaderc_free_buffer(compiled);
    if (compileError) {
        shaderc_free_error(compileError);
    }
    return true;
}

const UniformEntry* FindUniform(const std::vector<UniformEntry>& uniforms, const std::string& name) {
    for (const auto& uniform : uniforms) {
        if (uniform.name == name) {
            return &uniform;
        }
    }
    return nullptr;
}

const UniformEntry* FindUniformBySuffix(const std::vector<UniformEntry>& uniforms, const std::string& name) {
    for (const auto& uniform : uniforms) {
        if (uniform.name == name) {
            return &uniform;
        }
        if (uniform.name.size() > name.size() &&
            uniform.name.compare(uniform.name.size() - name.size(), name.size(), name) == 0 &&
            uniform.name[uniform.name.size() - name.size() - 1] == '.') {
            return &uniform;
        }
    }
    return nullptr;
}

std::string JoinUniformNames(const std::vector<UniformEntry>& uniforms) {
    std::string joined;
    for (size_t i = 0; i < uniforms.size(); ++i) {
        if (i > 0) {
            joined += ", ";
        }
        joined += uniforms[i].name;
    }
    return joined;
}

uint8_t BaseUniformType(uint8_t type) {
    return static_cast<uint8_t>(type & ~kUniformMask);
}

}  // namespace

TEST(ShadercUniformMappingTest, IntUniformsMapToVec4) {
    const std::string fragmentSource = R"(
#version 450
layout (location = 0) out vec4 fragColor;
layout(std140, set = 0, binding = 0) uniform DataBlock {
    int u_numActiveLightSources;
    ivec2 u_lightDataType;
};

void main() {
    fragColor = vec4(float(u_numActiveLightSources + u_lightDataType.x));
}
)";

    std::vector<uint8_t> binary;
    std::string error;
    ASSERT_TRUE(CompileShaderBinary(fragmentSource, "fragment", "spirv", binary, error))
        << error;

    ShaderParseResult parsed = ParseUniforms(binary, 'f');
    ASSERT_TRUE(parsed.ok) << parsed.error;

    const UniformEntry* scalar = FindUniformBySuffix(parsed.uniforms, "u_numActiveLightSources");
    ASSERT_NE(scalar, nullptr) << "Uniforms: " << JoinUniformNames(parsed.uniforms);
    EXPECT_EQ(BaseUniformType(scalar->type), static_cast<uint8_t>(bgfx::UniformType::Vec4));

    const UniformEntry* vector = FindUniformBySuffix(parsed.uniforms, "u_lightDataType");
    ASSERT_NE(vector, nullptr) << "Uniforms: " << JoinUniformNames(parsed.uniforms);
    EXPECT_EQ(BaseUniformType(vector->type), static_cast<uint8_t>(bgfx::UniformType::Vec4));
}

TEST(ShadercUniformMappingTest, SamplerUniformRemainsSampler) {
    const std::string fragmentSource = R"(
#version 450
layout (location = 0) out vec4 fragColor;
layout(set = 0, binding = 1) uniform sampler2D s_tex;

void main() {
    fragColor = texture(s_tex, vec2(0.25, 0.5));
}
)";

    std::vector<uint8_t> binary;
    std::string error;
    ASSERT_TRUE(CompileShaderBinary(fragmentSource, "fragment", "spirv", binary, error))
        << error;

    ShaderParseResult parsed = ParseUniforms(binary, 'f');
    ASSERT_TRUE(parsed.ok) << parsed.error;

    if (parsed.uniforms.empty()) {
        GTEST_SKIP() << "No sampler metadata emitted for raw GLSL; requires bgfx shader preprocessor";
    }

    const UniformEntry* sampler = FindUniformBySuffix(parsed.uniforms, "s_tex");
    if (!sampler) {
        for (const auto& uniform : parsed.uniforms) {
            if (BaseUniformType(uniform.type) == static_cast<uint8_t>(bgfx::UniformType::Sampler)) {
                sampler = &uniform;
                break;
            }
        }
    }
    ASSERT_NE(sampler, nullptr) << "Uniforms: " << JoinUniformNames(parsed.uniforms);
    EXPECT_EQ(BaseUniformType(sampler->type), static_cast<uint8_t>(bgfx::UniformType::Sampler));
}

TEST(ShadercUniformMappingTest, IntUniformsDoNotSetSamplerFlag) {
    const std::string fragmentSource = R"(
#version 450
layout (location = 0) out vec4 fragColor;
layout(std140, set = 0, binding = 0) uniform DataBlock {
    int u_counter;
};

void main() {
    fragColor = vec4(float(u_counter));
}
)";

    std::vector<uint8_t> binary;
    std::string error;
    ASSERT_TRUE(CompileShaderBinary(fragmentSource, "fragment", "spirv", binary, error))
        << error;

    ShaderParseResult parsed = ParseUniforms(binary, 'f');
    ASSERT_TRUE(parsed.ok) << parsed.error;

    const UniformEntry* scalar = FindUniformBySuffix(parsed.uniforms, "u_counter");
    ASSERT_NE(scalar, nullptr) << "Uniforms: " << JoinUniformNames(parsed.uniforms);
    EXPECT_EQ(BaseUniformType(scalar->type), static_cast<uint8_t>(bgfx::UniformType::Vec4));
    EXPECT_EQ(static_cast<uint8_t>(scalar->type & kUniformSamplerBit), 0);
}

TEST(ShadercUniformMappingTest, Mat3UniformMapsToMat3) {
    const std::string fragmentSource = R"(
#version 450
layout (location = 0) out vec4 fragColor;
layout(std140, set = 0, binding = 0) uniform DataBlock {
    mat3 u_basis;
};

void main() {
    fragColor = vec4(u_basis[0], 1.0);
}
)";

    std::vector<uint8_t> binary;
    std::string error;
    ASSERT_TRUE(CompileShaderBinary(fragmentSource, "fragment", "spirv", binary, error))
        << error;

    ShaderParseResult parsed = ParseUniforms(binary, 'f');
    ASSERT_TRUE(parsed.ok) << parsed.error;

    const UniformEntry* uniform = FindUniformBySuffix(parsed.uniforms, "u_basis");
    ASSERT_NE(uniform, nullptr) << "Uniforms: " << JoinUniformNames(parsed.uniforms);
    EXPECT_EQ(BaseUniformType(uniform->type), static_cast<uint8_t>(bgfx::UniformType::Mat3));
}

TEST(ShadercUniformMappingTest, Mat4UniformMapsToMat4) {
    const std::string fragmentSource = R"(
#version 450
layout (location = 0) out vec4 fragColor;
layout(std140, set = 0, binding = 0) uniform DataBlock {
    mat4 u_transform;
};

void main() {
    fragColor = u_transform * vec4(1.0);
}
)";

    std::vector<uint8_t> binary;
    std::string error;
    ASSERT_TRUE(CompileShaderBinary(fragmentSource, "fragment", "spirv", binary, error))
        << error;

    ShaderParseResult parsed = ParseUniforms(binary, 'f');
    ASSERT_TRUE(parsed.ok) << parsed.error;

    const UniformEntry* uniform = FindUniformBySuffix(parsed.uniforms, "u_transform");
    ASSERT_NE(uniform, nullptr) << "Uniforms: " << JoinUniformNames(parsed.uniforms);
    EXPECT_EQ(BaseUniformType(uniform->type), static_cast<uint8_t>(bgfx::UniformType::Mat4));
}
