#pragma once

#include "i_graphics_service.hpp"
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>

namespace sdl3cpp::services {

/**
 * @brief Shader management service interface.
 *
 * Handles shader compilation, pipeline creation, and shader program management.
 * Consumes shader paths from the shader script service.
 */
class IShaderService {
public:
    virtual ~IShaderService() = default;

    /**
     * @brief Register a shader program.
     *
     * @param key Unique identifier for this shader program
     * @param paths Vertex and fragment shader file paths
     * @throws std::runtime_error if shader files don't exist
     */
    virtual void RegisterShader(const std::string& key, const ShaderPaths& paths) = 0;

    /**
     * @brief Compile all registered shaders and create pipelines.
     *
     * Must be called after all shaders are registered and before rendering.
     *
     * @throws std::runtime_error if compilation fails
     */
    virtual void CompileAll() = 0;

    /**
     * @brief Get the Vulkan pipeline for a shader program.
     *
     * @param key Shader program identifier
     * @return VkPipeline handle
     * @throws std::out_of_range if shader key doesn't exist
     */
    virtual VkPipeline GetPipeline(const std::string& key) const = 0;

    /**
     * @brief Check if a shader program exists.
     *
     * @param key Shader program identifier
     * @return true if registered, false otherwise
     */
    virtual bool HasShader(const std::string& key) const = 0;

    /**
     * @brief Get the number of registered shader programs.
     *
     * @return Shader count
     */
    virtual size_t GetShaderCount() const = 0;

    /**
     * @brief Reload all shaders (for hot-reload support).
     *
     * Recompiles all shaders from their source files.
     *
     * @throws std::runtime_error if compilation fails
     */
    virtual void ReloadAll() = 0;
};

}  // namespace sdl3cpp::services
