#pragma once

#include "graphics_types.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

namespace sdl3cpp::services {
 *
 * Manages Vulkan graphics pipelines and shader compilation.
 * Small, focused service (~200 lines) for pipeline management.
 */
class IPipelineService {
public:
    virtual ~IPipelineService() = default;

    /**
     * @brief Register a shader program.
     *
     * @param key Unique identifier for the shader
     * @param paths Vertex and fragment shader file paths
     */
    virtual void RegisterShader(const std::string& key, const ShaderPaths& paths) = 0;

    /**
     * @brief Compile all registered shaders and create pipelines.
     *
     * @param renderPass Render pass for pipeline compatibility
     * @param extent Viewport extent
     * @throws std::runtime_error if compilation fails
     */
    virtual void CompileAll(VkRenderPass renderPass, VkExtent2D extent) = 0;

    /**
     * @brief Recreate pipelines (e.g., after swapchain recreation).
     *
     * @param renderPass New render pass
     * @param extent New viewport extent
     * @throws std::runtime_error if recreation fails
     */
    virtual void RecreatePipelines(VkRenderPass renderPass, VkExtent2D extent) = 0;

    /**
     * @brief Cleanup all pipelines.
     */
    virtual void Cleanup() = 0;

    /**
     * @brief Get a pipeline by shader key.
     *
     * @param key Shader identifier
     * @return VkPipeline handle
     * @throws std::out_of_range if key doesn't exist
     */
    virtual VkPipeline GetPipeline(const std::string& key) const = 0;

    /**
     * @brief Get the pipeline layout.
     *
     * @return VkPipelineLayout handle
     */
    virtual VkPipelineLayout GetPipelineLayout() const = 0;

    /**
     * @brief Check if a shader exists.
     *
     * @param key Shader identifier
     * @return true if registered, false otherwise
     */
    virtual bool HasShader(const std::string& key) const = 0;

    /**
     * @brief Get the number of registered shaders.
     *
     * @return Shader count
     */
    virtual size_t GetShaderCount() const = 0;
};

}  // namespace sdl3cpp::services
