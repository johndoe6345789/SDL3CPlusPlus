#include "shader_pipeline_validator.hpp"
#include "services/interfaces/i_logger.hpp"
#include <sstream>

namespace sdl3cpp::services {

std::vector<ShaderPipelineValidator::AttributeInfo>
ShaderPipelineValidator::ExtractShaderInputs(const std::string& glslSource) const {
    std::vector<AttributeInfo> inputs;

    // Match: layout (location = N) in type name; (handles compact syntax too)
    std::regex pattern(R"(layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s*in\s+(\w+)\s+(\w+)\s*;)");

    std::sregex_iterator begin(glslSource.begin(), glslSource.end(), pattern);
    std::sregex_iterator end;

    for (auto it = begin; it != end; ++it) {
        int location = std::stoi((*it)[1].str());
        std::string type = (*it)[2].str();
        std::string name = (*it)[3].str();
        size_t size = GetGlslTypeSize(type);

        inputs.emplace_back(location, type, name, size);

        // Trace logging disabled to avoid API mismatch
    }

    return inputs;
}

std::vector<ShaderPipelineValidator::AttributeInfo>
ShaderPipelineValidator::ExtractShaderOutputs(const std::string& glslSource) const {
    std::vector<AttributeInfo> outputs;

    // Match: layout (location = N) out type name; (handles compact syntax too)
    std::regex pattern(R"(layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s*out\s+(\w+)\s+(\w+)\s*;)");

    std::sregex_iterator begin(glslSource.begin(), glslSource.end(), pattern);
    std::sregex_iterator end;

    for (auto it = begin; it != end; ++it) {
        int location = std::stoi((*it)[1].str());
        std::string type = (*it)[2].str();
        std::string name = (*it)[3].str();
        size_t size = GetGlslTypeSize(type);

        outputs.emplace_back(location, type, name, size);
    }

    return outputs;
}

ShaderPipelineValidator::ValidationResult
ShaderPipelineValidator::ValidateVertexLayoutMatch(
    const std::vector<AttributeInfo>& shaderInputs,
    const std::vector<AttributeInfo>& vertexLayoutAttribs,
    const std::string& shaderName) const {

    ValidationResult result;

    // Build maps for quick lookup
    std::map<int, const AttributeInfo*> shaderMap;
    std::map<int, const AttributeInfo*> layoutMap;

    for (const auto& attr : shaderInputs) {
        shaderMap[attr.location] = &attr;
    }
    for (const auto& attr : vertexLayoutAttribs) {
        layoutMap[attr.location] = &attr;
    }

    // Check that all shader inputs are satisfied by layout
    for (const auto& [loc, shaderAttr] : shaderMap) {
        auto layoutIt = layoutMap.find(loc);
        if (layoutIt == layoutMap.end()) {
            result.AddError("Shader '" + shaderName + "' expects input at location " +
                          std::to_string(loc) + " (" + shaderAttr->name + ": " +
                          shaderAttr->type + ") but vertex layout doesn't provide it");
        } else {
            const AttributeInfo* layoutAttr = layoutIt->second;

            // Type compatibility check
            if (!TypesCompatible(shaderAttr->type, layoutAttr->type)) {
                result.AddError("Type mismatch at location " + std::to_string(loc) +
                              ": shader expects " + shaderAttr->type +
                              " but vertex layout provides " + layoutAttr->type);
            }

            // Size check
            if (shaderAttr->sizeBytes != layoutAttr->sizeBytes) {
                result.AddWarning("Size mismatch at location " + std::to_string(loc) +
                                ": shader expects " + std::to_string(shaderAttr->sizeBytes) +
                                " bytes but layout provides " + std::to_string(layoutAttr->sizeBytes) +
                                " bytes");
            }
        }
    }

    // Warn about unused layout attributes
    for (const auto& [loc, layoutAttr] : layoutMap) {
        if (shaderMap.find(loc) == shaderMap.end()) {
            result.AddWarning("Vertex layout provides unused attribute at location " +
                            std::to_string(loc) + " (" + layoutAttr->name + ")");
        }
    }

    return result;
}

ShaderPipelineValidator::ValidationResult
ShaderPipelineValidator::ValidateVertexStride(
    const std::vector<AttributeInfo>& layoutAttribs,
    size_t actualVertexStructSize) const {

    ValidationResult result;

    // Calculate expected stride from layout
    size_t expectedStride = 0;
    for (const auto& attr : layoutAttribs) {
        expectedStride += attr.sizeBytes;
    }

    if (expectedStride != actualVertexStructSize) {
        std::ostringstream oss;
        oss << "Vertex layout stride mismatch: layout expects " << expectedStride
            << " bytes but actual vertex struct is " << actualVertexStructSize << " bytes.\n";
        oss << "Layout breakdown:\n";
        for (const auto& attr : layoutAttribs) {
            oss << "  " << attr.name << " (" << attr.type << "): " << attr.sizeBytes << " bytes\n";
        }
        result.AddError(oss.str());
    }

    return result;
}

ShaderPipelineValidator::ValidationResult
ShaderPipelineValidator::ValidateInterfaceMatching(
    const std::vector<AttributeInfo>& vsOutputs,
    const std::vector<AttributeInfo>& fsInputs,
    const std::string& vsName,
    const std::string& fsName) const {

    ValidationResult result;

    std::map<int, const AttributeInfo*> vsMap;
    std::map<int, const AttributeInfo*> fsMap;

    for (const auto& attr : vsOutputs) {
        vsMap[attr.location] = &attr;
    }
    for (const auto& attr : fsInputs) {
        fsMap[attr.location] = &attr;
    }

    // Fragment shader inputs must be provided by vertex shader outputs
    for (const auto& [loc, fsAttr] : fsMap) {
        auto vsIt = vsMap.find(loc);
        if (vsIt == vsMap.end()) {
            result.AddError("Fragment shader '" + fsName + "' expects input at location " +
                          std::to_string(loc) + " (" + fsAttr->name + ") but vertex shader '" +
                          vsName + "' doesn't output it");
        } else {
            const AttributeInfo* vsAttr = vsIt->second;
            if (vsAttr->type != fsAttr->type) {
                result.AddError("Type mismatch at location " + std::to_string(loc) +
                              ": VS outputs " + vsAttr->type + " but FS expects " + fsAttr->type);
            }
        }
    }

    return result;
}

ShaderPipelineValidator::ValidationResult
ShaderPipelineValidator::ValidateSpirvRequirements(
    const std::string& glslSource,
    const std::string& shaderName) const {

    ValidationResult result;

    // Check that all in/out variables have layout(location=N)
    std::regex inOutPattern(R"(\b(in|out)\s+\w+\s+\w+\s*;)");
    std::regex layoutPattern(R"(layout\s*\()");

    size_t lineNum = 1;
    std::istringstream stream(glslSource);
    std::string line;

    while (std::getline(stream, line)) {
        // Skip version directives and built-ins
        if (line.find("#version") != std::string::npos ||
            line.find("gl_") != std::string::npos) {
            lineNum++;
            continue;
        }

        // Remove inline comments before checking
        std::string lineWithoutComments = line;
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            lineWithoutComments = line.substr(0, commentPos);
        }

        // Skip empty lines or pure comment lines
        if (lineWithoutComments.find_first_not_of(" \t\r\n") == std::string::npos) {
            lineNum++;
            continue;
        }

        // Check for in/out without layout
        std::smatch inOutMatch;
        if (std::regex_search(lineWithoutComments, inOutMatch, inOutPattern)) {
            if (!std::regex_search(lineWithoutComments, layoutPattern)) {
                result.AddError(std::string("SPIR-V requires 'layout(location=N)' for all inputs/outputs ") +
                              "at line " + std::to_string(lineNum) + " in shader '" + shaderName + "'");
            }
        }

        lineNum++;
    }

    return result;
}

ShaderPipelineValidator::ValidationResult
ShaderPipelineValidator::ValidatePipeline(
    const std::string& vertexShaderSource,
    const std::string& fragmentShaderSource,
    const std::vector<AttributeInfo>& vertexLayoutAttribs,
    size_t actualVertexStructSize,
    const std::string& pipelineName) const {

    ValidationResult combined;

    // Extract attributes
    auto vsInputs = ExtractShaderInputs(vertexShaderSource);
    auto vsOutputs = ExtractShaderOutputs(vertexShaderSource);
    auto fsInputs = ExtractShaderInputs(fragmentShaderSource);

    // Run all validations
    auto layoutMatch = ValidateVertexLayoutMatch(vsInputs, vertexLayoutAttribs,
                                                  pipelineName + ":vertex");
    auto strideCheck = ValidateVertexStride(vertexLayoutAttribs, actualVertexStructSize);
    auto interfaceMatch = ValidateInterfaceMatching(vsOutputs, fsInputs,
                                                    pipelineName + ":vertex",
                                                    pipelineName + ":fragment");
    auto spirvVs = ValidateSpirvRequirements(vertexShaderSource, pipelineName + ":vertex");
    auto spirvFs = ValidateSpirvRequirements(fragmentShaderSource, pipelineName + ":fragment");

    // Combine results
    auto mergeResults = [&](const ValidationResult& r) {
        if (!r.passed) combined.passed = false;
        combined.errors.insert(combined.errors.end(), r.errors.begin(), r.errors.end());
        combined.warnings.insert(combined.warnings.end(), r.warnings.begin(), r.warnings.end());
    };

    mergeResults(layoutMatch);
    mergeResults(strideCheck);
    mergeResults(interfaceMatch);
    mergeResults(spirvVs);
    mergeResults(spirvFs);

    return combined;
}

void ShaderPipelineValidator::LogValidationResult(
    const ValidationResult& result,
    const std::string& context) const {

    if (!logger_) return;

    if (result.passed && result.warnings.empty()) {
        logger_->Info("[" + context + "] ✓ Validation passed");
        return;
    }

    for (const auto& error : result.errors) {
        logger_->Error("[" + context + "] ✗ " + error);
    }

    for (const auto& warning : result.warnings) {
        logger_->Warn("[" + context + "] ⚠ " + warning);
    }

    if (!result.passed) {
        logger_->Error("[" + context + "] ❌ Validation FAILED with " +
                      std::to_string(result.errors.size()) + " errors");
    }
}

} // namespace sdl3cpp::services
