#include "bgfx_shader_compiler.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdlib>

#include "shaderc_mem.h"

namespace sdl3cpp::services::impl {

namespace {

const char* RendererTypeName(bgfx::RendererType::Enum type) {
    switch (type) {
        case bgfx::RendererType::Vulkan: return "Vulkan";
        case bgfx::RendererType::OpenGL: return "OpenGL";
        case bgfx::RendererType::OpenGLES: return "OpenGLES";
        case bgfx::RendererType::Direct3D11: return "Direct3D11";
        case bgfx::RendererType::Direct3D12: return "Direct3D12";
        case bgfx::RendererType::Metal: return "Metal";
        case bgfx::RendererType::Noop: return "Noop";
        default: return "Unknown";
    }
}

const char* ShadercTargetName(bgfx::RendererType::Enum type) {
    switch (type) {
        case bgfx::RendererType::Vulkan: return "spirv";
        case bgfx::RendererType::Metal: return "msl";
        case bgfx::RendererType::Direct3D11:
        case bgfx::RendererType::Direct3D12: return "hlsl";
        case bgfx::RendererType::OpenGL:
        case bgfx::RendererType::OpenGLES: return "glsl";
        default: return "spirv";
    }
}

}  // namespace

BgfxShaderCompiler::BgfxShaderCompiler(std::shared_ptr<ILogger> logger,
                                       std::shared_ptr<sdl3cpp::services::IPipelineCompilerService> pipelineCompiler)
    : logger_(std::move(logger)), pipelineCompiler_(std::move(pipelineCompiler)) {
}

bgfx::ShaderHandle BgfxShaderCompiler::CompileShader(
    const std::string& label,
    const std::string& source,
    bool isVertex,
    const std::vector<BgfxShaderUniform>& uniforms,
    const std::vector<bgfx::Attrib::Enum>& attributes) const {

    const bgfx::RendererType::Enum rendererType = bgfx::getRendererType();

    if (logger_) {
        logger_->Trace("BgfxShaderCompiler", "CompileShader",
                       "label=" + label +
                       ", renderer=" + std::string(RendererTypeName(rendererType)) +
                       ", sourceLength=" + std::to_string(source.size()) +
                       ", uniforms=" + std::to_string(uniforms.size()));
    }

    // For OpenGL: bgfx expects raw GLSL source
    const bool isOpenGL = (rendererType == bgfx::RendererType::OpenGL || 
                           rendererType == bgfx::RendererType::OpenGLES);
    
    if (isOpenGL) {
        const uint32_t sourceSize = static_cast<uint32_t>(source.size());
        const bgfx::Memory* mem = bgfx::copy(source.c_str(), sourceSize + 1);
        
        bgfx::ShaderHandle handle = bgfx::createShader(mem);
        if (!bgfx::isValid(handle) && logger_) {
            logger_->Error("BgfxShaderCompiler: Failed to create OpenGL shader for " + label);
        }
        return handle;
    }

    // Try in-memory in-process compilation first (if bgfx_tools provides C API)
    std::vector<char> buffer;
    bool compiledInMemory = false;

    const char* profile = isVertex ? "vertex" : "fragment";
    const char* target = ShadercTargetName(rendererType);
    uint8_t* outData = nullptr;
    size_t outSize = 0;
    char* outError = nullptr;

    int result = shaderc_compile_from_memory_with_target(
        source.c_str(),
        source.size(),
        profile,
        target,
        &outData,
        &outSize,
        &outError);

    if (result == 0 && outData && outSize > 0) {
        buffer.resize(outSize);
        memcpy(buffer.data(), outData, outSize);
        compiledInMemory = true;
        if (logger_) {
            logger_->Trace("BgfxShaderCompiler", "CompileShader",
                           "in-memory compile succeeded for " + label +
                           ", target=" + std::string(target) +
                           ", size=" + std::to_string(outSize));
        }
    } else if (logger_) {
        logger_->Trace("BgfxShaderCompiler", "CompileShader",
                       "in-memory compile failed for " + label +
                       ", target=" + std::string(target) +
                       ", result=" + std::to_string(result) +
                       ", size=" + std::to_string(outSize));
        if (outError) {
            logger_->Error(std::string("BgfxShaderCompiler: in-memory shaderc error: ") + outError);
        }
    }

    if (outData) {
        shaderc_free_buffer(outData);
    }
    if (outError) {
        shaderc_free_error(outError);
    }

    if (!compiledInMemory) {
        if (logger_) {
            logger_->Trace("BgfxShaderCompiler", "CompileShader",
                           "falling back to temp-file compilation for " + label);
        }
        // Fallback to temp-file + pipelineCompiler_/executable flow
        std::string tempInputPath = "/tmp/" + label + (isVertex ? ".vert.glsl" : ".frag.glsl");
        std::string tempOutputPath = "/tmp/" + label + (isVertex ? ".vert.bin" : ".frag.bin");
        {
            std::ofstream ofs(tempInputPath);
            ofs << source;
        }

        bool ok = false;
        if (pipelineCompiler_) {
            std::vector<std::string> args = {"--type", isVertex ? "vertex" : "fragment", "--profile", "spirv"};
            ok = pipelineCompiler_->Compile(tempInputPath, tempOutputPath, args);
        } else {
            std::string cmd = "./src/bgfx_tools/shaderc/shaderc -f " + tempInputPath + " -o " + tempOutputPath;
            if (logger_) logger_->Trace("BgfxShaderCompiler", "CompileShaderCmd", cmd);
            int rc = std::system(cmd.c_str());
            ok = (rc == 0);
        }

        if (!ok) {
            if (logger_) logger_->Error("BgfxShaderCompiler: shader compilation failed for " + label);
            return BGFX_INVALID_HANDLE;
        }

        std::ifstream ifs(tempOutputPath, std::ios::binary | std::ios::ate);
        if (!ifs) {
            if (logger_) logger_->Error("BgfxShaderCompiler: Failed to read compiled shader: " + tempOutputPath);
            return BGFX_INVALID_HANDLE;
        }
        std::streamsize size = ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        buffer.resize(size);
        if (!ifs.read(buffer.data(), size)) {
            if (logger_) logger_->Error("BgfxShaderCompiler: Failed to read compiled shader data");
            return BGFX_INVALID_HANDLE;
        }
        // cleanup temp files
        remove(tempInputPath.c_str());
        remove(tempOutputPath.c_str());
    }

    uint32_t binSize = static_cast<uint32_t>(buffer.size());
    const bgfx::Memory* mem = bgfx::copy(buffer.data(), binSize);
    bgfx::ShaderHandle handle = bgfx::createShader(mem);
    if (!bgfx::isValid(handle) && logger_) {
        logger_->Error("BgfxShaderCompiler: bgfx::createShader failed for " + label +
                      " (binSize=" + std::to_string(binSize) + ")");
    } else if (logger_) {
        logger_->Trace("BgfxShaderCompiler", "CompileShader",
                       "label=" + label + " shader created successfully, handle=" + std::to_string(handle.idx));
    }

    return handle;
}

}  // namespace sdl3cpp::services::impl
