#pragma once

#include "../interfaces/i_pipeline_service.hpp"
#include "../interfaces/i_vulkan_device_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../../di/lifecycle.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Pipeline service implementation.
 *
 * Small, focused service (~200 lines) for Vulkan graphics pipeline management.
 * Handles shader compilation, pipeline creation, and pipeline layout.
 */
class PipelineService : public IPipelineService,
                        public di::IShutdownable {
public:
    explicit PipelineService(std::shared_ptr<IVulkanDeviceService> deviceService, std::shared_ptr<ILogger> logger);
    ~PipelineService() override;

    // IPipelineService interface
    void RegisterShader(const std::string& key, const ShaderPaths& paths) override;
    void CompileAll(VkRenderPass renderPass, VkExtent2D extent) override;
    void RecreatePipelines(VkRenderPass renderPass, VkExtent2D extent) override;
    void Cleanup() override;

    VkPipeline GetPipeline(const std::string& key) const override;
    VkPipelineLayout GetPipelineLayout() const override {
        logger_->Trace("PipelineService", "GetPipelineLayout");
        return pipelineLayout_;
    }
    bool HasShader(const std::string& key) const override;
    size_t GetShaderCount() const override {
        logger_->Trace("PipelineService", "GetShaderCount");
        return shaderPathMap_.size();
    }

    // IShutdownable interface
    void Shutdown() noexcept override;

private:
    std::shared_ptr<IVulkanDeviceService> deviceService_;
    std::shared_ptr<ILogger> logger_;

    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    std::unordered_map<std::string, ShaderPaths> shaderPathMap_;
    std::unordered_map<std::string, VkPipeline> pipelines_;

    // Helper methods
    VkShaderModule CreateShaderModule(const std::vector<char>& code);
    std::vector<char> ReadShaderFile(const std::string& path);
    void CreatePipelineLayout();
    void CreatePipelinesInternal(VkRenderPass renderPass, VkExtent2D extent);
    void CleanupPipelines();
};

}  // namespace sdl3cpp::services::impl
