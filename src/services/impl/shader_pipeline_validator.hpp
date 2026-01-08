#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <memory>

namespace sdl3cpp::services {

// Forward declarations
class ILogger;

/**
 * Compile-time and runtime shader pipeline validator
 * Catches mismatches between shaders, vertex layouts, and vertex data
 * BEFORE submitting to the GPU driver
 */
class ShaderPipelineValidator {
public:
    struct AttributeInfo {
        int location;
        std::string type;      // "vec3", "vec2", etc.
        std::string name;
        size_t sizeBytes;      // Expected size in bytes

        AttributeInfo(int loc, const std::string& t, const std::string& n, size_t sz)
            : location(loc), type(t), name(n), sizeBytes(sz) {}
    };

    struct ValidationResult {
        bool passed;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;

        void AddError(const std::string& msg) {
            errors.push_back(msg);
            passed = false;
        }

        void AddWarning(const std::string& msg) {
            warnings.push_back(msg);
        }

        ValidationResult() : passed(true) {}
    };

    explicit ShaderPipelineValidator(std::shared_ptr<ILogger> logger = nullptr)
        : logger_(std::move(logger)) {}

    // Extract attribute information from GLSL source
    std::vector<AttributeInfo> ExtractShaderInputs(const std::string& glslSource) const;
    std::vector<AttributeInfo> ExtractShaderOutputs(const std::string& glslSource) const;

    // Validate that vertex layout matches shader expectations
    ValidationResult ValidateVertexLayoutMatch(
        const std::vector<AttributeInfo>& shaderInputs,
        const std::vector<AttributeInfo>& vertexLayoutAttribs,
        const std::string& shaderName) const;

    // Validate that vertex struct size matches layout stride
    ValidationResult ValidateVertexStride(
        const std::vector<AttributeInfo>& layoutAttribs,
        size_t actualVertexStructSize) const;

    // Validate that vertex shader outputs match fragment shader inputs
    ValidationResult ValidateInterfaceMatching(
        const std::vector<AttributeInfo>& vsOutputs,
        const std::vector<AttributeInfo>& fsInputs,
        const std::string& vsName,
        const std::string& fsName) const;

    // Validate SPIR-V requirements
    ValidationResult ValidateSpirvRequirements(
        const std::string& glslSource,
        const std::string& shaderName) const;

    // Comprehensive validation - runs all checks
    ValidationResult ValidatePipeline(
        const std::string& vertexShaderSource,
        const std::string& fragmentShaderSource,
        const std::vector<AttributeInfo>& vertexLayoutAttribs,
        size_t actualVertexStructSize,
        const std::string& pipelineName) const;

    // Log validation result
    void LogValidationResult(const ValidationResult& result, const std::string& context) const;

private:
    std::shared_ptr<ILogger> logger_;

    // Helper to get expected size for a GLSL type
    size_t GetGlslTypeSize(const std::string& glslType) const {
        static const std::map<std::string, size_t> sizes = {
            {"float", 4},
            {"vec2", 8},
            {"vec3", 12},
            {"vec4", 16},
            {"int", 4},
            {"ivec2", 8},
            {"ivec3", 12},
            {"ivec4", 16},
            {"uint", 4},
            {"uvec2", 8},
            {"uvec3", 12},
            {"uvec4", 16},
        };

        auto it = sizes.find(glslType);
        return it != sizes.end() ? it->second : 0;
    }

    bool TypesCompatible(const std::string& shaderType, const std::string& layoutType) const {
        // Add compatibility mappings
        if (shaderType == layoutType) return true;

        // vec3 <-> float3 etc
        std::map<std::string, std::string> aliases = {
            {"vec2", "float2"}, {"vec3", "float3"}, {"vec4", "float4"},
            {"ivec2", "int2"}, {"ivec3", "int3"}, {"ivec4", "int4"},
        };

        auto it = aliases.find(shaderType);
        return it != aliases.end() && it->second == layoutType;
    }
};

} // namespace sdl3cpp::services
