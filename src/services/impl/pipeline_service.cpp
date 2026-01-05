#include "pipeline_service.hpp"
#include "../../core/vertex.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace sdl3cpp::services::impl {

PipelineService::PipelineService(std::shared_ptr<IVulkanDeviceService> deviceService, std::shared_ptr<ILogger> logger)
    : deviceService_(std::move(deviceService)), logger_(logger) {
    if (logger_) {
        logger_->Trace("PipelineService", "PipelineService",
                       "deviceService=" + std::string(deviceService_ ? "set" : "null"));
    }
}

PipelineService::~PipelineService() {
    if (logger_) {
        logger_->Trace("PipelineService", "~PipelineService");
    }
    if (pipelineLayout_ != VK_NULL_HANDLE || !pipelines_.empty()) {
        Shutdown();
    }
}

void PipelineService::RegisterShader(const std::string& key, const ShaderPaths& paths) {
    logger_->Trace("PipelineService", "RegisterShader",
                   "key=" + key +
                   ", vertex=" + paths.vertex +
                   ", fragment=" + paths.fragment);
    shaderPathMap_[key] = paths;
    logger_->Debug("Registered shader: " + key);
}

void PipelineService::CompileAll(VkRenderPass renderPass, VkExtent2D extent) {
    logger_->Trace("PipelineService", "CompileAll",
                   "renderPassIsNull=" + std::string(renderPass == VK_NULL_HANDLE ? "true" : "false") +
                   ", extent.width=" + std::to_string(extent.width) +
                   ", extent.height=" + std::to_string(extent.height));

    if (shaderPathMap_.empty()) {
        throw std::runtime_error("No shader paths were registered before pipeline creation");
    }

    CreatePipelineLayout();
    CreatePipelinesInternal(renderPass, extent);

    logger_->Info("Compiled " + std::to_string(pipelines_.size()) + " pipeline(s)");
}

void PipelineService::RecreatePipelines(VkRenderPass renderPass, VkExtent2D extent) {
    logger_->Trace("PipelineService", "RecreatePipelines",
                   "renderPassIsNull=" + std::string(renderPass == VK_NULL_HANDLE ? "true" : "false") +
                   ", extent.width=" + std::to_string(extent.width) +
                   ", extent.height=" + std::to_string(extent.height));

    CleanupPipelines();
    CreatePipelineLayout();
    CreatePipelinesInternal(renderPass, extent);

    logger_->Info("Recreated " + std::to_string(pipelines_.size()) + " pipeline(s)");
}

void PipelineService::Cleanup() {
    logger_->Trace("PipelineService", "Cleanup");
    CleanupPipelines();

    auto device = deviceService_->GetDevice();

    if (pipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
    }
}

void PipelineService::Shutdown() noexcept {
    logger_->Trace("PipelineService", "Shutdown");
    Cleanup();
}

VkPipeline PipelineService::GetPipeline(const std::string& key) const {
    logger_->Trace("PipelineService", "GetPipeline", "key=" + key);
    auto it = pipelines_.find(key);
    if (it == pipelines_.end()) {
        throw std::out_of_range("Pipeline not found: " + key);
    }
    return it->second;
}

bool PipelineService::HasShader(const std::string& key) const {
    logger_->Trace("PipelineService", "HasShader", "key=" + key);
    return shaderPathMap_.find(key) != shaderPathMap_.end();
}

void PipelineService::CreatePipelineLayout() {
    logger_->Trace("PipelineService", "CreatePipelineLayout");

    auto device = deviceService_->GetDevice();

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(core::PushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;

    if (pipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
    }

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }
}

void PipelineService::CreatePipelinesInternal(VkRenderPass renderPass, VkExtent2D extent) {
    logger_->Trace("PipelineService", "CreatePipelinesInternal",
                   "renderPassIsNull=" + std::string(renderPass == VK_NULL_HANDLE ? "true" : "false") +
                   ", extent.width=" + std::to_string(extent.width) +
                   ", extent.height=" + std::to_string(extent.height));

    auto device = deviceService_->GetDevice();

    // Vertex input configuration
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(core::Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(core::Vertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(core::Vertex, color);

    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Rasterization
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Base pipeline info
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout_;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    // Create pipeline for each registered shader
    for (const auto& [key, paths] : shaderPathMap_) {
        // Validate shader files exist
        if (!std::filesystem::exists(paths.vertex)) {
            throw std::runtime_error(
                "Vertex shader not found: " + paths.vertex +
                "\n\nShader key: " + key +
                "\n\nPlease ensure shader files are compiled and present in the shaders directory.");
        }
        if (!std::filesystem::exists(paths.fragment)) {
            throw std::runtime_error(
                "Fragment shader not found: " + paths.fragment +
                "\n\nShader key: " + key +
                "\n\nPlease ensure shader files are compiled and present in the shaders directory.");
        }

        auto vertShaderCode = ReadShaderFile(paths.vertex);
        auto fragShaderCode = ReadShaderFile(paths.fragment);

        VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertStageInfo{};
        vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStageInfo.module = vertShaderModule;
        vertStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragStageInfo{};
        fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStageInfo.module = fragShaderModule;
        fragStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

        VkGraphicsPipelineCreateInfo pipelineCreateInfo = pipelineInfo;
        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages = shaderStages;

        VkPipeline pipeline;
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
                                      &pipeline) != VK_SUCCESS) {
            vkDestroyShaderModule(device, fragShaderModule, nullptr);
            vkDestroyShaderModule(device, vertShaderModule, nullptr);
            throw std::runtime_error("Failed to create graphics pipeline for shader: " + key);
        }

        pipelines_[key] = pipeline;

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);

        logger_->Debug("Created pipeline: " + key);
    }
}

void PipelineService::CleanupPipelines() {
    logger_->Trace("PipelineService", "CleanupPipelines");

    auto device = deviceService_->GetDevice();

    for (auto& [key, pipeline] : pipelines_) {
        vkDestroyPipeline(device, pipeline, nullptr);
    }
    pipelines_.clear();
}

VkShaderModule PipelineService::CreateShaderModule(const std::vector<char>& code) {
    logger_->Trace("PipelineService", "CreateShaderModule",
                   "code.size=" + std::to_string(code.size()));

    auto device = deviceService_->GetDevice();

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }
    return shaderModule;
}

std::vector<char> PipelineService::ReadShaderFile(const std::string& path) {
    logger_->Trace("PipelineService", "ReadShaderFile", "path=" + path);

    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Shader file not found: " + path +
            "\n\nPlease ensure the file exists at this location.");
    }

    if (!std::filesystem::is_regular_file(path)) {
        throw std::runtime_error("Path is not a regular file: " + path);
    }

    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open shader file: " + path +
            "\n\nCheck file permissions.");
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
    file.close();

    logger_->Debug("Read shader file: " + path + " (" + std::to_string(fileSize) + " bytes)");

    return buffer;
}

}  // namespace sdl3cpp::services::impl
