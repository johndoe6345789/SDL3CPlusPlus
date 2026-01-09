#include "bgfx_shader_compiler.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <optional>
#include <cstdlib>

#include "shaderc_mem.h"

namespace sdl3cpp::services::impl {

namespace {

constexpr uint8_t kUniformFragmentBit = 0x10;
constexpr uint8_t kUniformSamplerBit = 0x20;
constexpr uint8_t kUniformReadOnlyBit = 0x40;
constexpr uint8_t kUniformCompareBit = 0x80;
constexpr uint8_t kUniformMask = kUniformFragmentBit |
                                 kUniformSamplerBit |
                                 kUniformReadOnlyBit |
                                 kUniformCompareBit;

struct ShaderBinaryUniform {
    std::string name;
    uint8_t type = 0;
    uint8_t num = 0;
    uint16_t regIndex = 0;
    uint16_t regCount = 0;
    std::optional<uint8_t> texComponent;
    std::optional<uint8_t> texDimension;
    std::optional<uint16_t> texFormat;
};

struct ShaderBinaryInfo {
    uint32_t magic = 0;
    uint8_t version = 0;
    char shaderType = '?';
    uint32_t hashIn = 0;
    uint32_t hashOut = 0;
    bool oldBindingModel = false;
    std::vector<ShaderBinaryUniform> uniforms;
    uint32_t shaderSize = 0;
    bool spirvMagicOk = false;
    std::vector<uint16_t> attributes;
    uint16_t constantSize = 0;
};

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

const char* UniformTypeName(uint8_t type) {
    switch (type) {
        case bgfx::UniformType::Sampler:
            return "Sampler";
        case bgfx::UniformType::Vec4:
            return "Vec4";
        case bgfx::UniformType::Mat3:
            return "Mat3";
        case bgfx::UniformType::Mat4:
            return "Mat4";
        case bgfx::UniformType::End:
            return "End";
        default:
            return "Unknown";
    }
}

std::string FormatUniformFlags(uint8_t type) {
    std::string flags;
    auto appendFlag = [&flags](const char* value) {
        if (!flags.empty()) {
            flags += "|";
        }
        flags += value;
    };
    if ((type & kUniformFragmentBit) != 0) {
        appendFlag("fragment");
    }
    if ((type & kUniformSamplerBit) != 0) {
        appendFlag("sampler");
    }
    if ((type & kUniformReadOnlyBit) != 0) {
        appendFlag("readonly");
    }
    if ((type & kUniformCompareBit) != 0) {
        appendFlag("compare");
    }
    if (flags.empty()) {
        flags = "none";
    }
    return flags;
}

std::string EncodeBase64(const uint8_t* data, size_t size) {
    static constexpr char kAlphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    if (!data || size == 0) {
        return output;
    }

    output.reserve(((size + 2) / 3) * 4);
    for (size_t index = 0; index < size; index += 3) {
        const uint32_t byte0 = data[index];
        const uint32_t byte1 = (index + 1 < size) ? data[index + 1] : 0;
        const uint32_t byte2 = (index + 2 < size) ? data[index + 2] : 0;
        const uint32_t triple = (byte0 << 16) | (byte1 << 8) | byte2;

        output.push_back(kAlphabet[(triple >> 18) & 0x3F]);
        output.push_back(kAlphabet[(triple >> 12) & 0x3F]);
        output.push_back((index + 1 < size) ? kAlphabet[(triple >> 6) & 0x3F] : '=');
        output.push_back((index + 2 < size) ? kAlphabet[triple & 0x3F] : '=');
    }
    return output;
}

bool ParseBgfxShaderBinary(const std::vector<char>& buffer,
                           char expectedType,
                           ShaderBinaryInfo& info,
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

    if (!readBytes(&info.magic, sizeof(info.magic))) {
        error = "failed to read magic";
        return false;
    }

    const uint32_t base = info.magic & 0x00FFFFFFu;
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

    info.shaderType = (base == vsh) ? 'v' : (base == fsh ? 'f' : 'c');
    info.version = static_cast<uint8_t>(info.magic >> 24);
    info.oldBindingModel = info.version < 11;
    const bool hasTexData = info.version >= 8;
    const bool hasTexFormat = info.version >= 10;

    if (!readBytes(&info.hashIn, sizeof(info.hashIn)) ||
        !readBytes(&info.hashOut, sizeof(info.hashOut))) {
        error = "failed to read hashes";
        return false;
    }

    uint16_t uniformCount = 0;
    if (!readBytes(&uniformCount, sizeof(uniformCount))) {
        error = "failed to read uniform count";
        return false;
    }

    info.uniforms.clear();
    info.uniforms.reserve(uniformCount);
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
        ShaderBinaryUniform uniform;
        uniform.name.assign(buffer.data() + offset, buffer.data() + offset + nameSize);
        offset += nameSize;

        if (!readBytes(&uniform.type, sizeof(uniform.type)) ||
            !readBytes(&uniform.num, sizeof(uniform.num)) ||
            !readBytes(&uniform.regIndex, sizeof(uniform.regIndex)) ||
            !readBytes(&uniform.regCount, sizeof(uniform.regCount))) {
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
            uniform.texComponent = texComponent;
            uniform.texDimension = texDimension;
        }
        if (hasTexFormat) {
            uint16_t texFormat = 0;
            if (!readBytes(&texFormat, sizeof(texFormat))) {
                error = "failed to read texture format";
                return false;
            }
            uniform.texFormat = texFormat;
        }
        info.uniforms.push_back(std::move(uniform));
    }

    if (!readBytes(&info.shaderSize, sizeof(info.shaderSize))) {
        error = "failed to read shader size";
        return false;
    }
    if (info.shaderSize % 4 != 0) {
        error = "shader size not aligned";
        return false;
    }
    if (offset + info.shaderSize + 1 > buffer.size()) {
        error = "shader code out of bounds";
        return false;
    }

    info.spirvMagicOk = false;
    if (info.shaderSize >= sizeof(uint32_t)) {
        uint32_t spirvMagic = 0;
        std::memcpy(&spirvMagic, buffer.data() + offset, sizeof(uint32_t));
        info.spirvMagicOk = (spirvMagic == 0x07230203u);
    }

    offset += info.shaderSize + 1;

    uint8_t numAttrs = 0;
    if (!readBytes(&numAttrs, sizeof(numAttrs))) {
        error = "failed to read attribute count";
        return false;
    }
    if (offset + static_cast<size_t>(numAttrs) * sizeof(uint16_t) > buffer.size()) {
        error = "attribute list out of bounds";
        return false;
    }
    info.attributes.clear();
    info.attributes.reserve(numAttrs);
    for (uint8_t i = 0; i < numAttrs; ++i) {
        uint16_t attrId = 0;
        if (!readBytes(&attrId, sizeof(attrId))) {
            error = "failed to read attribute id";
            return false;
        }
        info.attributes.push_back(attrId);
    }

    if (!readBytes(&info.constantSize, sizeof(info.constantSize))) {
        error = "failed to read constant size";
        return false;
    }

    return true;
}

void LogShaderBinaryInfo(const std::shared_ptr<ILogger>& logger,
                         const std::string& label,
                         const ShaderBinaryInfo& info) {
    if (!logger) {
        return;
    }

    std::string attrList;
    for (size_t i = 0; i < info.attributes.size(); ++i) {
        if (i > 0) {
            attrList += ",";
        }
        attrList += std::to_string(info.attributes[i]);
    }
    if (attrList.empty()) {
        attrList = "none";
    }

    logger->Trace("BgfxShaderCompiler", "ShaderBinaryInfo",
                  "label=" + label +
                  ", type=" + std::string(1, info.shaderType) +
                  ", version=" + std::to_string(info.version) +
                  ", oldBindingModel=" + std::to_string(info.oldBindingModel) +
                  ", hashIn=" + std::to_string(info.hashIn) +
                  ", hashOut=" + std::to_string(info.hashOut) +
                  ", uniforms=" + std::to_string(info.uniforms.size()) +
                  ", shaderSize=" + std::to_string(info.shaderSize) +
                  ", attrCount=" + std::to_string(info.attributes.size()) +
                  ", attrIds=[" + attrList + "]" +
                  ", constantSize=" + std::to_string(info.constantSize) +
                  ", spirvMagicOk=" + std::to_string(info.spirvMagicOk));

    for (const auto& uniform : info.uniforms) {
        const uint8_t baseType = static_cast<uint8_t>(uniform.type & ~kUniformMask);
        logger->Trace("BgfxShaderCompiler", "ShaderUniform",
                      "label=" + label +
                      ", name=" + uniform.name +
                      ", baseType=" + std::string(UniformTypeName(baseType)) +
                      ", flags=" + FormatUniformFlags(uniform.type) +
                      ", num=" + std::to_string(uniform.num) +
                      ", regIndex=" + std::to_string(uniform.regIndex) +
                      ", regCount=" + std::to_string(uniform.regCount) +
                      ", texComponent=" + std::to_string(uniform.texComponent.value_or(0)) +
                      ", texDimension=" + std::to_string(uniform.texDimension.value_or(0)) +
                      ", texFormat=" + std::to_string(uniform.texFormat.value_or(0)));
    }
}

void LogShaderBinaryBase64(const std::shared_ptr<ILogger>& logger,
                           const std::string& label,
                           const std::vector<char>& buffer) {
    if (!logger || buffer.empty()) {
        return;
    }
    const size_t loggedBytes = buffer.size();
    const bool truncated = loggedBytes < buffer.size();

    const auto* raw = reinterpret_cast<const uint8_t*>(buffer.data());
    std::string encoded = EncodeBase64(raw, loggedBytes);

    logger->Trace("BgfxShaderCompiler", "ShaderBinaryBase64",
                  "label=" + label +
                  ", bytes=" + std::to_string(buffer.size()) +
                  ", loggedBytes=" + std::to_string(loggedBytes) +
                  ", truncated=" + std::to_string(truncated) +
                  ", base64=" + encoded);
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
    (void)attributes;

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
    std::optional<ShaderBinaryInfo> shaderInfo;

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
        ShaderBinaryInfo parsedInfo;
        std::string validationError;
        if (!ParseBgfxShaderBinary(buffer, isVertex ? 'v' : 'f', parsedInfo, validationError)) {
            compiledInMemory = false;
            buffer.clear();
            if (logger_) {
                logger_->Error("BgfxShaderCompiler: invalid shader binary for " + label +
                               " (" + validationError + ")");
            }
        } else {
            shaderInfo = std::move(parsedInfo);
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
        if (size <= 0) {
            if (logger_) {
                logger_->Error("BgfxShaderCompiler: Compiled shader size invalid for " + label);
            }
            return BGFX_INVALID_HANDLE;
        }
        buffer.resize(static_cast<size_t>(size));
        if (!ifs.read(buffer.data(), size)) {
            if (logger_) logger_->Error("BgfxShaderCompiler: Failed to read compiled shader data");
            return BGFX_INVALID_HANDLE;
        }
        // cleanup temp files
        remove(tempInputPath.c_str());
        remove(tempOutputPath.c_str());
    }

    if (!shaderInfo.has_value()) {
        ShaderBinaryInfo parsedInfo;
        std::string validationError;
        if (!ParseBgfxShaderBinary(buffer, isVertex ? 'v' : 'f', parsedInfo, validationError)) {
            if (logger_) {
                logger_->Error("BgfxShaderCompiler: invalid shader binary for " + label +
                               " (" + validationError + ")");
            }
            return BGFX_INVALID_HANDLE;
        }
        shaderInfo = std::move(parsedInfo);
    }

    if (shaderInfo.has_value()) {
        LogShaderBinaryInfo(logger_, label, *shaderInfo);
        LogShaderBinaryBase64(logger_, label, buffer);
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
