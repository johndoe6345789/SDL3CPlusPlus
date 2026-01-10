#include "pipeline_compiler_service.hpp"
#include <cstdlib>
#include <sstream>
#include <vector>
#include <optional>
#include <string>
#include <memory>
#include <utility>
#include <fstream>
#include <iterator>
#include <cstring>
#include <shaderc/shaderc.hpp>

namespace sdl3cpp::services::impl {

PipelineCompilerService::PipelineCompilerService(std::shared_ptr<sdl3cpp::services::ILogger> logger)
    : logger_(std::move(logger)) {}

bool PipelineCompilerService::Compile(const std::string& inputPath,
                                      const std::string& outputPath,
                                      const std::vector<std::string>& args) {
    logger_->Trace("PipelineCompilerService", "Compile", "Called with input=" + inputPath + ", output=" + outputPath);
    // Parse args to determine shader type and profile
    bool isVertex = false;
    std::string profile = "spirv"; // default
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--type" && i + 1 < args.size()) {
            isVertex = (args[i + 1] == "vertex");
        } else if (args[i] == "--profile" && i + 1 < args.size()) {
            profile = args[i + 1];
        }
    }

    // Read input file
    std::ifstream inputFile(inputPath);
    if (!inputFile) {
        lastError_ = "Failed to open input file: " + inputPath;
        logger_->Error(lastError_.value());
        return false;
    }
    std::string source((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());

    // For bgfx Vulkan shaders, use the source as-is (it has proper Vulkan syntax)
    std::string processedSource = source;

    // Write output
    std::ofstream outputFile(outputPath, std::ios::binary);
    if (!outputFile) {
        lastError_ = "Failed to open output file: " + outputPath;
        logger_->Error(lastError_.value());
        return false;
    }

    if (profile == "glsl") {
        // For GLSL, just copy the source
        outputFile.write(source.c_str(), static_cast<std::streamsize>(source.size()));
    } else {
        // For SPIR-V, compile using shaderc library
        shaderc_shader_kind kind = isVertex ? shaderc_vertex_shader : shaderc_fragment_shader;

        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        // Set target environment to Vulkan for Vulkan renderer
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0);
        options.SetTargetSpirv(shaderc_spirv_version_1_0);

        shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(processedSource, kind, inputPath.c_str(), options);

        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
            lastError_ = "Shader compilation failed: " + result.GetErrorMessage();
            logger_->Error(lastError_.value());
            return false;
        }

        // Get the SPIR-V binary data
        const uint32_t* spirvData = result.begin();
        const size_t spirvWords = static_cast<size_t>(result.end() - result.begin());
        const size_t spirvSize = spirvWords * sizeof(uint32_t);

        // Write bgfx shader header
        const uint32_t shaderBinVersion = 11;
        uint32_t chunkMagic = isVertex 
            ? (('V' << 0) | ('S' << 8) | ('H' << 16) | (shaderBinVersion << 24))
            : (('F' << 0) | ('S' << 8) | ('H' << 16) | (shaderBinVersion << 24));
        
        // Write chunk magic
        outputFile.write(reinterpret_cast<const char*>(&chunkMagic), sizeof(chunkMagic));
        
        // Write hash (use 0 for now, bgfx might compute this)
        uint32_t hash = 0;
        outputFile.write(reinterpret_cast<const char*>(&hash), sizeof(hash));
        
        // Write SPIR-V data
        outputFile.write(reinterpret_cast<const char*>(spirvData), static_cast<std::streamsize>(spirvSize));

        // Write uniform information (bgfx expects this after the SPIR-V)
        if (isVertex) {
            // Vertex shader uniforms: UniformBuffer.u_modelViewProj (mat4)
            uint16_t numUniforms = 1;
            outputFile.write(reinterpret_cast<const char*>(&numUniforms), sizeof(numUniforms));
            
            // Uniform name (null-terminated) - include block name for Vulkan
            const char* uniformName = "UniformBuffer.u_modelViewProj";
            outputFile.write(uniformName, static_cast<std::streamsize>(strlen(uniformName) + 1));
            
            // Uniform type (Mat4 = 4)
            uint8_t uniformType = 4;
            outputFile.write(reinterpret_cast<const char*>(&uniformType), sizeof(uniformType));
            
            // num elements
            uint8_t num = 1;
            outputFile.write(reinterpret_cast<const char*>(&num), sizeof(num));
            
            // regIndex, regCount
            uint16_t regIndex = 0;
            uint16_t regCount = 1;
            outputFile.write(reinterpret_cast<const char*>(&regIndex), sizeof(regIndex));
            outputFile.write(reinterpret_cast<const char*>(&regCount), sizeof(regCount));
            
            // Texture info (not used for mat4)
            uint8_t texComponent = 0;
            uint8_t texDimension = 0;
            uint16_t texFormat = 0;
            outputFile.write(reinterpret_cast<const char*>(&texComponent), sizeof(texComponent));
            outputFile.write(reinterpret_cast<const char*>(&texDimension), sizeof(texDimension));
            outputFile.write(reinterpret_cast<const char*>(&texFormat), sizeof(texFormat));
        } else {
            // Fragment shader uniforms: s_tex (sampler2D)
            uint16_t numUniforms = 1;
            outputFile.write(reinterpret_cast<const char*>(&numUniforms), sizeof(numUniforms));
            
            // Uniform name (null-terminated)
            const char* uniformName = "s_tex";
            outputFile.write(uniformName, static_cast<std::streamsize>(strlen(uniformName) + 1));
            
            // Uniform type (Sampler = 5)
            uint8_t uniformType = 5;
            outputFile.write(reinterpret_cast<const char*>(&uniformType), sizeof(uniformType));
            
            // num elements
            uint8_t num = 1;
            outputFile.write(reinterpret_cast<const char*>(&num), sizeof(num));
            
            // regIndex, regCount
            uint16_t regIndex = 1; // s_tex is at index 1
            uint16_t regCount = 1;
            outputFile.write(reinterpret_cast<const char*>(&regIndex), sizeof(regIndex));
            outputFile.write(reinterpret_cast<const char*>(&regCount), sizeof(regCount));
            
            // Texture info
            uint8_t texComponent = 0;
            uint8_t texDimension = 2; // 2D texture
            uint16_t texFormat = 0;
            outputFile.write(reinterpret_cast<const char*>(&texComponent), sizeof(texComponent));
            outputFile.write(reinterpret_cast<const char*>(&texDimension), sizeof(texDimension));
            outputFile.write(reinterpret_cast<const char*>(&texFormat), sizeof(texFormat));
        }
    }

    logger_->Trace("PipelineCompilerService", "Compile", "Successfully compiled " + inputPath + " to " + outputPath);
    lastError_.reset();
    return true;
}

std::optional<std::string> PipelineCompilerService::GetLastError() const {
    return lastError_;
}
} // namespace sdl3cpp::services::impl
