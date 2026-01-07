#include "bgfx_shader_compiler.hpp"

#include <shaderc/shaderc.hpp>
#include <algorithm>
#include <cstring>
#include <numeric>
#include <stdexcept>

namespace sdl3cpp::services::impl {

namespace {

constexpr uint8_t kUniformFragmentBit = 0x10;
constexpr uint8_t kUniformMask = 0x10 | 0x20 | 0x40 | 0x80;

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

uint16_t WriteUniformArray(uint8_t* data,
                           uint32_t& offset,
                           const std::vector<BgfxShaderUniform>& uniforms,
                           bool isFragmentShader) {
    uint16_t size = 0;
    const uint16_t count = static_cast<uint16_t>(uniforms.size());
    std::memcpy(data + offset, &count, sizeof(count));
    offset += sizeof(count);

    const uint8_t fragmentBit = isFragmentShader ? kUniformFragmentBit : 0;
    for (const auto& un : uniforms) {
        if ((static_cast<uint8_t>(un.type) & ~kUniformMask) > bgfx::UniformType::End) {
            size = std::max<uint16_t>(size, static_cast<uint16_t>(un.regIndex + un.regCount * 16));
        }

        const uint8_t nameSize = static_cast<uint8_t>(un.name.size());
        std::memcpy(data + offset, &nameSize, sizeof(nameSize));
        offset += sizeof(nameSize);
        std::memcpy(data + offset, un.name.data(), nameSize);
        offset += nameSize;

        const uint8_t typeByte = static_cast<uint8_t>(un.type) | fragmentBit;
        std::memcpy(data + offset, &typeByte, sizeof(typeByte));
        offset += sizeof(typeByte);
        std::memcpy(data + offset, &un.num, sizeof(un.num));
        offset += sizeof(un.num);
        std::memcpy(data + offset, &un.regIndex, sizeof(un.regIndex));
        offset += sizeof(un.regIndex);
        std::memcpy(data + offset, &un.regCount, sizeof(un.regCount));
        offset += sizeof(un.regCount);
        std::memcpy(data + offset, &un.texComponent, sizeof(un.texComponent));
        offset += sizeof(un.texComponent);
        std::memcpy(data + offset, &un.texDimension, sizeof(un.texDimension));
        offset += sizeof(un.texDimension);
        std::memcpy(data + offset, &un.texFormat, sizeof(un.texFormat));
        offset += sizeof(un.texFormat);
    }
    return size;
}

uint16_t AttributeToId(bgfx::Attrib::Enum attr) {
    switch (attr) {
        case bgfx::Attrib::Position: return 0x0001;
        case bgfx::Attrib::Color0: return 0x0005;
        case bgfx::Attrib::TexCoord0: return 0x0010;
        case bgfx::Attrib::Normal: return 0x0002;
        case bgfx::Attrib::Tangent: return 0x0003;
        case bgfx::Attrib::Bitangent: return 0x0004;
        default: return 0xFFFF;
    }
}

}  // namespace

BgfxShaderCompiler::BgfxShaderCompiler(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
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
    
    // For Vulkan/Metal/DX: Compile GLSL to SPIRV
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);
    options.SetAutoBindUniforms(false);  // We use explicit binding=N in shaders
    // Do NOT use SetAutoMapLocations - it overrides explicit layout(location=N) declarations
    
    shaderc_shader_kind kind = isVertex ? shaderc_vertex_shader : shaderc_fragment_shader;
    auto result = compiler.CompileGlslToSpv(source, kind, label.c_str(), options);
    
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        if (logger_) {
            logger_->Error("BgfxShaderCompiler: GLSL->SPIRV compilation failed for " + label + "\n" + result.GetErrorMessage());
        }
        return BGFX_INVALID_HANDLE;
    }

    std::vector<uint32_t> spirv(result.cbegin(), result.cend());
    
    if (logger_) {
        logger_->Trace("BgfxShaderCompiler", "CompileShader",
                       "label=" + label + " SPIRV compiled, " + std::to_string(spirv.size()) + " words");
    }
    
    // Wrap SPIRV with bgfx binary format (simple version without uniform metadata embedding)
    // bgfx can extract uniform info from SPIRV reflection, so we don't need to embed it
    constexpr uint8_t kBgfxShaderVersion = 11;
    constexpr uint32_t kMagicVSH = ('V') | ('S' << 8) | ('H' << 16) | (kBgfxShaderVersion << 24);
    constexpr uint32_t kMagicFSH = ('F') | ('S' << 8) | ('H' << 16) | (kBgfxShaderVersion << 24);
    const uint32_t magic = isVertex ? kMagicVSH : kMagicFSH;
    const uint32_t inputHash = static_cast<uint32_t>(std::hash<std::string>{}(source));
    const uint32_t spirvSize = static_cast<uint32_t>(spirv.size() * sizeof(uint32_t));
    const uint16_t uniformCount = 0;  // Let bgfx extract uniforms from SPIRV
    const uint32_t totalSize = 4 + 4 + 4 + 2 + 4 + spirvSize + 1;
    
    const bgfx::Memory* mem = bgfx::alloc(totalSize);
    uint8_t* data = mem->data;
    uint32_t offset = 0;
    
    std::memcpy(data + offset, &magic, 4); offset += 4;
    std::memcpy(data + offset, &inputHash, 4); offset += 4;
    std::memcpy(data + offset, &inputHash, 4); offset += 4;  // outputHash = inputHash
    std::memcpy(data + offset, &uniformCount, 2); offset += 2;
    std::memcpy(data + offset, &spirvSize, 4); offset += 4;
    std::memcpy(data + offset, spirv.data(), spirvSize); offset += spirvSize;
    data[offset] = 0;  // null terminator
    
    bgfx::ShaderHandle handle = bgfx::createShader(mem);
    if (!bgfx::isValid(handle) && logger_) {
        logger_->Error("BgfxShaderCompiler: bgfx::createShader failed for " + label +
                      " (spirvSize=" + std::to_string(spirv.size()) + " words)");
    } else if (logger_) {
        logger_->Trace("BgfxShaderCompiler", "CompileShader",
                       "label=" + label + " shader created successfully, handle=" + std::to_string(handle.idx));
    }
    
    return handle;
}

}  // namespace sdl3cpp::services::impl
