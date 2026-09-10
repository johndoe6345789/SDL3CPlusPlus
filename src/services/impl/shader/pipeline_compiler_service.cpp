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

    // The source is already valid Vulkan GLSL, so use it as-is.
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

        // The engine loads SPIR-V straight into SDL_CreateGPUShader, so write the
        // raw module with no container around it.
        outputFile.write(reinterpret_cast<const char*>(spirvData),
                         static_cast<std::streamsize>(spirvSize));
    }

    logger_->Trace("PipelineCompilerService", "Compile", "Successfully compiled " + inputPath + " to " + outputPath);
    lastError_.reset();
    return true;
}

std::optional<std::string> PipelineCompilerService::GetLastError() const {
    return lastError_;
}
} // namespace sdl3cpp::services::impl
