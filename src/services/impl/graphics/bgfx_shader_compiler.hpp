#pragma once

#include "services/interfaces/i_logger.hpp"
#include <bgfx/bgfx.h>
#include <memory>
#include <string>
#include <vector>
#include "services/interfaces/i_pipeline_compiler_service.hpp"

namespace sdl3cpp::services::impl {

/**
 * @brief Uniform metadata for bgfx shader binary format.
 */
struct BgfxShaderUniform {
    std::string name;
    bgfx::UniformType::Enum type = bgfx::UniformType::Count;
    uint8_t num = 0;
    uint16_t regIndex = 0;
    uint16_t regCount = 0;
    uint8_t texComponent = 0;
    uint8_t texDimension = 0;
    uint16_t texFormat = 0;
};

/**
 * @brief Shared shader compilation service for bgfx.
 * 
 * Centralizes GLSL->SPIRV compilation and bgfx binary wrapping.
 * Ensures uniform metadata is correctly embedded for all backends (Vulkan, Metal, DX).
 */
class BgfxShaderCompiler {
public:
    explicit BgfxShaderCompiler(std::shared_ptr<ILogger> logger,
                                std::shared_ptr<sdl3cpp::services::IPipelineCompilerService> pipelineCompiler = nullptr);
    
    /**
     * @brief Compile GLSL source to bgfx shader handle.
     * 
     * For OpenGL: passes GLSL source directly to bgfx.
     * For Vulkan/Metal/DX: compiles to SPIRV and wraps in bgfx binary format with uniform metadata.
     * 
     * @param label Debug label for error messages
     * @param source GLSL shader source code
     * @param isVertex True for vertex shader, false for fragment shader
     * @param uniforms Uniform metadata to embed (Vulkan/Metal/DX only)
     * @param attributes Vertex attributes (vertex shaders only)
     * @return bgfx::ShaderHandle (invalid on failure)
     */
    bgfx::ShaderHandle CompileShader(const std::string& label,
                                     const std::string& source,
                                     bool isVertex,
                                     const std::vector<BgfxShaderUniform>& uniforms,
                                     const std::vector<bgfx::Attrib::Enum>& attributes = {}) const;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<sdl3cpp::services::IPipelineCompilerService> pipelineCompiler_;
};

}  // namespace sdl3cpp::services::impl
