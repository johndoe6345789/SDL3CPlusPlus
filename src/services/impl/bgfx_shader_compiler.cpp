#include "bgfx_shader_compiler.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdlib>
// For runtime symbol lookup
#if defined(__linux__) || defined(__APPLE__)
#include <dlfcn.h>
#endif

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

#if defined(__linux__) || defined(__APPLE__)
    using shaderc_fn_t = int (*)(const char*, size_t, const char*, uint8_t**, size_t*, char**);
    using shaderc_with_target_fn_t = int (*)(const char*, size_t, const char*, const char*, uint8_t**, size_t*, char**);
    void* sym_with_target = dlsym(RTLD_DEFAULT, "shaderc_compile_from_memory_with_target");
    shaderc_with_target_fn_t shaderc_with_target_fn = reinterpret_cast<shaderc_with_target_fn_t>(sym_with_target);

    void* sym = dlsym(RTLD_DEFAULT, "shaderc_compile_from_memory");
    shaderc_fn_t shaderc_fn = reinterpret_cast<shaderc_fn_t>(sym);

    if (shaderc_with_target_fn || shaderc_fn) {
        uint8_t* out_data = nullptr;
        size_t out_size = 0;
        char* out_err = nullptr;
        // profile: choose based on vertex/fragment and renderer; simple heuristic
        const char* profile = isVertex ? "vertex" : "fragment";
        int r = -1;
        if (shaderc_with_target_fn) {
            // choose target based on renderer
            const char* target = "spirv";
            switch (rendererType) {
                case bgfx::RendererType::Vulkan: target = "spirv"; break;
                case bgfx::RendererType::Metal: target = "msl"; break;
                case bgfx::RendererType::Direct3D11:
                case bgfx::RendererType::Direct3D12: target = "hlsl"; break;
                case bgfx::RendererType::OpenGL:
                case bgfx::RendererType::OpenGLES: target = "glsl"; break;
                default: target = "spirv"; break;
            }
            r = shaderc_with_target_fn(source.c_str(), source.size(), profile, target, &out_data, &out_size, &out_err);
        } else if (shaderc_fn) {
            r = shaderc_fn(source.c_str(), source.size(), profile, &out_data, &out_size, &out_err);
        }
        if (r == 0 && out_data && out_size > 0) {
            buffer.resize(out_size);
            memcpy(buffer.data(), out_data, out_size);
            // free using provided free if available
            // try to find free function
            void* free_sym = dlsym(RTLD_DEFAULT, "shaderc_free_buffer");
            if (free_sym) {
                using free_fn_t = void (*)(uint8_t*);
                reinterpret_cast<free_fn_t>(free_sym)(out_data);
            } else {
                free(out_data);
            }
            if (out_err) {
                void* free_err_sym = dlsym(RTLD_DEFAULT, "shaderc_free_error");
                if (free_err_sym) {
                    using free_err_fn_t = void (*)(char*);
                    reinterpret_cast<free_err_fn_t>(free_err_sym)(out_err);
                } else {
                    free(out_err);
                }
            }
            compiledInMemory = true;
            if (logger_) logger_->Trace("BgfxShaderCompiler", "CompileShader", "in-memory compile succeeded for " + label);
        } else {
            if (out_err && logger_) {
                logger_->Error(std::string("BgfxShaderCompiler: in-memory shaderc error: ") + out_err);
                void* free_err_sym = dlsym(RTLD_DEFAULT, "shaderc_free_error");
                if (free_err_sym) {
                    using free_err_fn_t = void (*)(char*);
                    reinterpret_cast<free_err_fn_t>(free_err_sym)(out_err);
                } else {
                    free(out_err);
                }
            }
        }
    }
#endif

    if (!compiledInMemory) {
        // Fallback to temp-file + pipelineCompiler_/executable flow
        std::string tempInputPath = "/tmp/" + label + (isVertex ? ".vert.glsl" : ".frag.glsl");
        std::string tempOutputPath = "/tmp/" + label + (isVertex ? ".vert.bin" : ".frag.bin");
        {
            std::ofstream ofs(tempInputPath);
            ofs << source;
        }

        bool ok = false;
        if (pipelineCompiler_) {
            ok = pipelineCompiler_->Compile(tempInputPath, tempOutputPath, {});
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
