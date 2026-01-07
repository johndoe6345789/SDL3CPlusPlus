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
#include <shaderc_mem.h>

namespace sdl3cpp::services::impl {

PipelineCompilerService::PipelineCompilerService(std::shared_ptr<sdl3cpp::services::ILogger> logger)
    : logger_(std::move(logger)) {}

bool PipelineCompilerService::Compile(const std::string& inputPath,
                                      const std::string& outputPath,
                                      const std::vector<std::string>& args) {
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

    // Write output
    std::ofstream outputFile(outputPath, std::ios::binary);
    if (!outputFile) {
        lastError_ = "Failed to open output file: " + outputPath;
        logger_->Error(lastError_.value());
        return false;
    }

    if (profile == "glsl") {
        // For GLSL, just copy the source
        outputFile.write(source.c_str(), source.size());
    } else {
        // For SPIR-V, compile using bgfx_tools shaderc_mem
        const char* shaderType = isVertex ? "vertex" : "fragment";
        
        uint8_t* compiledData = nullptr;
        size_t compiledSize = 0;
        char* errorMsg = nullptr;
        
        int result = shaderc_compile_from_memory(source.c_str(), source.size(), shaderType, 
                                                &compiledData, &compiledSize, &errorMsg);
        
        if (result != 0) {
            std::string error = errorMsg ? errorMsg : "Unknown shader compilation error";
            lastError_ = "Shader compilation failed: " + error;
            logger_->Error(lastError_.value());
            if (errorMsg) shaderc_free_error(errorMsg);
            return false;
        }
        
        // Write the compiled bgfx binary data directly
        outputFile.write(reinterpret_cast<const char*>(compiledData), compiledSize);
        
        // Clean up
        shaderc_free_buffer(compiledData);
        if (errorMsg) shaderc_free_error(errorMsg);
    }

    logger_->Trace("PipelineCompilerService", "Compile", "Successfully compiled " + inputPath + " to " + outputPath);
    lastError_.reset();
    return true;
}

std::optional<std::string> PipelineCompilerService::GetLastError() const {
    return lastError_;
}
} // namespace sdl3cpp::services::impl
