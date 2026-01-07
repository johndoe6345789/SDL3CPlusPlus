#include "bgfx_shader_compiler.hpp"

#include <algorithm>
#include <cstdint>
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

bool ValidateBgfxShaderBinary(const std::vector<char>& buffer,
                              char expectedType,
                              std::string& error) {
    if (buffer.size() < sizeof(uint32_t) * 3 + sizeof(uint16_t)) {
        error = "buffer too small for header";
        return false;
    }

    size_t offset = 0;
    auto readBytes = [&](void* out, size_t size) {
        if (offset + size > buffer.size()) {
            return false;
        }
        std::memcpy(out, buffer.data() + offset, size);
        offset += size;
        return true;
    };

    uint32_t magic = 0;
    if (!readBytes(&magic, sizeof(magic))) {
        error = "failed to read magic";
        return false;
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
    const uint32_t expectedBase = (expectedType == 'v') ? vsh : (expectedType == 'c' ? csh : fsh);
    if (base != vsh && base != fsh && base != csh) {
        error = "invalid magic";
        return false;
    }
    if (base != expectedBase) {
        error = "shader type mismatch";
        return false;
    }

    const uint8_t version = static_cast<uint8_t>(magic >> 24);
    const bool hasTexData = version >= 8;
    const bool hasTexFormat = version >= 10;

    uint32_t hashIn = 0;
    uint32_t hashOut = 0;
    if (!readBytes(&hashIn, sizeof(hashIn)) || !readBytes(&hashOut, sizeof(hashOut))) {
        error = "failed to read hashes";
        return false;
    }

    uint16_t uniformCount = 0;
    if (!readBytes(&uniformCount, sizeof(uniformCount))) {
        error = "failed to read uniform count";
        return false;
    }

    for (uint16_t i = 0; i < uniformCount; ++i) {
        uint8_t nameSize = 0;
        if (!readBytes(&nameSize, sizeof(nameSize))) {
            error = "failed to read uniform name size";
            return false;
        }
        if (offset + nameSize > buffer.size()) {
            error = "uniform name out of bounds";
            return false;
        }
        offset += nameSize;

        uint8_t type = 0;
        uint8_t num = 0;
        uint16_t regIndex = 0;
        uint16_t regCount = 0;
        if (!readBytes(&type, sizeof(type)) ||
            !readBytes(&num, sizeof(num)) ||
            !readBytes(&regIndex, sizeof(regIndex)) ||
            !readBytes(&regCount, sizeof(regCount))) {
            error = "failed to read uniform metadata";
            return false;
        }

        if (hasTexData) {
            uint8_t texComponent = 0;
            uint8_t texDimension = 0;
            if (!readBytes(&texComponent, sizeof(texComponent)) ||
                !readBytes(&texDimension, sizeof(texDimension))) {
                error = "failed to read texture metadata";
                return false;
            }
        }
        if (hasTexFormat) {
            uint16_t texFormat = 0;
            if (!readBytes(&texFormat, sizeof(texFormat))) {
                error = "failed to read texture format";
                return false;
            }
        }
    }

    uint32_t shaderSize = 0;
    if (!readBytes(&shaderSize, sizeof(shaderSize))) {
        error = "failed to read shader size";
        return false;
    }
    if (shaderSize % 4 != 0) {
        error = "shader size not aligned";
        return false;
    }
    if (offset + shaderSize + 1 > buffer.size()) {
        error = "shader code out of bounds";
        return false;
    }

    if (shaderSize >= sizeof(uint32_t)) {
        uint32_t spirvMagic = 0;
        std::memcpy(&spirvMagic, buffer.data() + offset, sizeof(uint32_t));
        if (spirvMagic != 0x07230203u) {
            error = "invalid SPIR-V magic";
            return false;
        }
    }

    offset += shaderSize + 1;

    uint8_t numAttrs = 0;
    if (!readBytes(&numAttrs, sizeof(numAttrs))) {
        error = "failed to read attribute count";
        return false;
    }
    if (offset + static_cast<size_t>(numAttrs) * sizeof(uint16_t) > buffer.size()) {
        error = "attribute list out of bounds";
        return false;
    }
    offset += static_cast<size_t>(numAttrs) * sizeof(uint16_t);

    uint16_t constantSize = 0;
    if (!readBytes(&constantSize, sizeof(constantSize))) {
        error = "failed to read constant size";
        return false;
    }

    return true;
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
        std::string validationError;
        if (!ValidateBgfxShaderBinary(buffer, isVertex ? 'v' : 'f', validationError)) {
            compiledInMemory = false;
            buffer.clear();
            if (logger_) {
                logger_->Error("BgfxShaderCompiler: invalid shader binary for " + label +
                               " (" + validationError + ")");
            }
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

    std::string validationError;
    if (!ValidateBgfxShaderBinary(buffer, isVertex ? 'v' : 'f', validationError)) {
        if (logger_) {
            logger_->Error("BgfxShaderCompiler: invalid shader binary for " + label +
                           " (" + validationError + ")");
        }
        return BGFX_INVALID_HANDLE;
    }

    uint32_t binSize = static_cast<uint32_t>(buffer.size());
    const bgfx::Memory* mem = bgfx::copy(buffer.data(), binSize);
    bgfx::ShaderHandle handle = bgfx::createShader(mem);
    if (!bgfx::isValid(handle) && logger_) {
        logger_->Error("BgfxShaderCompiler: bgfx::createShader failed for " + label +
                      " (binSize=" + std::to_string(binSize) + ")");
    } else if (logger_) {
        logger_->Info("BgfxShaderCompiler: created shader " + label +
                      " (binSize=" + std::to_string(binSize) +
                      ", renderer=" + std::string(RendererTypeName(rendererType)) + ")");
        logger_->Trace("BgfxShaderCompiler", "CompileShader",
                       "label=" + label + " shader created successfully, handle=" + std::to_string(handle.idx));
    }

    return handle;
}

}  // namespace sdl3cpp::services::impl
