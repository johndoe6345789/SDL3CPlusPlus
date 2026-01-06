#include "pipeline_service.hpp"
#include "../../core/vertex.hpp"
#include <shaderc/shaderc.hpp>
#include <array>
#include <cstring>
#include <filesystem>
#include <functional>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace {
shaderc_shader_kind ShadercKindFromStage(VkShaderStageFlagBits stage) {
    switch (stage) {
        case VK_SHADER_STAGE_VERTEX_BIT:
            return shaderc_vertex_shader;
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            return shaderc_fragment_shader;
        case VK_SHADER_STAGE_GEOMETRY_BIT:
            return shaderc_geometry_shader;
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
            return shaderc_tess_control_shader;
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
            return shaderc_tess_evaluation_shader;
        case VK_SHADER_STAGE_COMPUTE_BIT:
            return shaderc_compute_shader;
        default:
            return shaderc_glsl_infer_from_source;
    }
}
}  // namespace

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
    std::string vertexLabel = paths.vertex.empty()
        ? (paths.vertexSource.empty() ? "<missing>" : "<inline>")
        : paths.vertex;
    std::string fragmentLabel = paths.fragment.empty()
        ? (paths.fragmentSource.empty() ? "<missing>" : "<inline>")
        : paths.fragment;
    logger_->Trace("PipelineService", "RegisterShader",
                   "key=" + key +
                   ", vertex=" + vertexLabel +
                   ", fragment=" + fragmentLabel);
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
    shaderSpirvCache_.clear();

    auto device = deviceService_->GetDevice();

    if (pipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
    }
    if (descriptorSetLayout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout_, nullptr);
        descriptorSetLayout_ = VK_NULL_HANDLE;
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

    if (descriptorSetLayout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout_, nullptr);
        descriptorSetLayout_ = VK_NULL_HANDLE;
    }
    if (pipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
    }

    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0;
    samplerBinding.descriptorCount = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = 1;
    setLayoutInfo.pBindings = &samplerBinding;

    if (vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &descriptorSetLayout_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout");
    }

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(core::PushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;

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

    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

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

    // Depth stencil
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};

    // Base pipeline info
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout_;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    // Create pipeline for each registered shader
    for (const auto& [key, paths] : shaderPathMap_) {
        auto requireShader = [&](const std::string& label,
                                 const std::string& path,
                                 const std::string& source) {
            if (!HasShaderSource(path, source)) {
                std::string labelPath = path.empty() ? "<inline>" : path;
                throw std::runtime_error(
                    label + " shader not found: " + labelPath +
                    "\n\nShader key: " + key +
                    "\n\nPlease ensure the shader source (.vert/.frag/etc.) exists.");
            }
        };

        // Validate shader files exist
        requireShader("Vertex", paths.vertex, paths.vertexSource);
        requireShader("Fragment", paths.fragment, paths.fragmentSource);

        bool hasGeometry = !paths.geometry.empty() || !paths.geometrySource.empty();
        bool hasTessControl = !paths.tessControl.empty() || !paths.tessControlSource.empty();
        bool hasTessEval = !paths.tessEval.empty() || !paths.tessEvalSource.empty();

        if (hasGeometry) {
            requireShader("Geometry", paths.geometry, paths.geometrySource);
        }
        if (hasTessControl != hasTessEval) {
            throw std::runtime_error(
                "Tessellation shaders require both 'tesc' and 'tese' paths. Shader key: " + key);
        }
        if (hasTessControl) {
            requireShader("Tessellation control", paths.tessControl, paths.tessControlSource);
        }
        if (hasTessEval) {
            requireShader("Tessellation evaluation", paths.tessEval, paths.tessEvalSource);
        }

        VkPipelineRasterizationStateCreateInfo rasterizerState = rasterizer;
        VkPipelineDepthStencilStateCreateInfo depthStencilState = depthStencil;
        if (paths.disableCulling) {
            rasterizerState.cullMode = VK_CULL_MODE_NONE;
        }
        if (paths.disableDepthTest) {
            depthStencilState.depthTestEnable = VK_FALSE;
            depthStencilState.depthWriteEnable = VK_FALSE;
        }
        if (logger_ && (paths.disableCulling || paths.disableDepthTest)) {
            logger_->Trace("PipelineService", "CreatePipelinesInternal",
                           "shaderKey=" + key +
                           ", cullMode=" + std::string(paths.disableCulling ? "none" : "back") +
                           ", depthTest=" + std::string(paths.disableDepthTest ? "off" : "on"));
        }

        std::vector<VkShaderModule> shaderModules;
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        shaderStages.reserve(2 + (hasGeometry ? 1 : 0) + (hasTessControl ? 2 : 0));

        auto destroyShaderModules = [&]() {
            for (VkShaderModule module : shaderModules) {
                vkDestroyShaderModule(device, module, nullptr);
            }
            shaderModules.clear();
        };

        auto addStage = [&](VkShaderStageFlagBits stage,
                            const std::string& path,
                            const std::string& source) {
            const auto& shaderCode = ReadShaderSource(path, source, stage);
            VkShaderModule shaderModule = CreateShaderModule(shaderCode);
            shaderModules.push_back(shaderModule);

            VkPipelineShaderStageCreateInfo stageInfo{};
            stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stageInfo.stage = stage;
            stageInfo.module = shaderModule;
            stageInfo.pName = "main";
            shaderStages.push_back(stageInfo);
        };

        try {
            addStage(VK_SHADER_STAGE_VERTEX_BIT, paths.vertex, paths.vertexSource);
            if (hasTessControl) {
                addStage(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, paths.tessControl,
                         paths.tessControlSource);
                addStage(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, paths.tessEval,
                         paths.tessEvalSource);
            }
            if (hasGeometry) {
                addStage(VK_SHADER_STAGE_GEOMETRY_BIT, paths.geometry, paths.geometrySource);
            }
            addStage(VK_SHADER_STAGE_FRAGMENT_BIT, paths.fragment, paths.fragmentSource);

            VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = inputAssembly;
            VkPipelineTessellationStateCreateInfo tessellationState{};
            bool useTessellation = hasTessControl && hasTessEval;
            if (useTessellation) {
                inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
                tessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
                tessellationState.patchControlPoints = 3;
            }

            VkGraphicsPipelineCreateInfo pipelineCreateInfo = pipelineInfo;
            pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
            pipelineCreateInfo.pTessellationState = useTessellation ? &tessellationState : nullptr;
            pipelineCreateInfo.pRasterizationState = &rasterizerState;
            pipelineCreateInfo.pDepthStencilState = &depthStencilState;
            pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
            pipelineCreateInfo.pStages = shaderStages.data();

            VkPipeline pipeline;
            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
                                          &pipeline) != VK_SUCCESS) {
                destroyShaderModules();
                throw std::runtime_error("Failed to create graphics pipeline for shader: " + key);
            }

            pipelines_[key] = pipeline;
            destroyShaderModules();
        } catch (...) {
            destroyShaderModules();
            throw;
        }

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

std::string PipelineService::BuildShaderCacheKey(const std::string& path,
                                                 const std::string& source,
                                                 VkShaderStageFlagBits stage) const {
    if (!source.empty()) {
        size_t hash = std::hash<std::string>{}(source);
        return "inline:" + std::to_string(hash) + "|" +
            std::to_string(static_cast<int>(stage));
    }
    std::filesystem::path shaderPath(path);
    if (shaderPath.extension() == ".spv") {
        shaderPath.replace_extension();
    }
    return shaderPath.string() + "|" +
        std::to_string(static_cast<int>(stage));
}

bool PipelineService::HasShaderSource(const std::string& path, const std::string& source) const {
    if (!source.empty()) {
        return true;
    }
    if (path.empty()) {
        return false;
    }
    std::filesystem::path shaderPath(path);
    if (shaderPath.extension() == ".spv") {
        shaderPath.replace_extension();
    }
    return std::filesystem::exists(shaderPath);
}

const std::vector<char>& PipelineService::ReadShaderSource(const std::string& path,
                                                           const std::string& source,
                                                           VkShaderStageFlagBits stage) {
    logger_->Trace("PipelineService", "ReadShaderSource",
                   "path=" + path + ", stage=" + std::to_string(static_cast<int>(stage)) +
                   ", source=" + std::string(source.empty() ? "path" : "inline"));

    if (path.empty() && source.empty()) {
        throw std::runtime_error("Shader path and source are empty");
    }

    const std::string cacheKey = BuildShaderCacheKey(path, source, stage);
    auto cached = shaderSpirvCache_.find(cacheKey);
    if (cached != shaderSpirvCache_.end()) {
        logger_->Trace("PipelineService", "ReadShaderSource",
                       "cacheHit=true, bytes=" + std::to_string(cached->second.size()));
        return cached->second;
    }

    std::string shaderLabel = path.empty() ? "<inline>" : path;
    std::string shaderSource = source;
    if (shaderSource.empty()) {
        std::filesystem::path shaderPath(path);
        if (shaderPath.extension() == ".spv") {
            std::filesystem::path sourcePath = shaderPath;
            sourcePath.replace_extension();
            logger_->Trace("PipelineService", "ReadShaderSource",
                           "usingSource=" + sourcePath.string());
            shaderPath = sourcePath;
        }

        if (!std::filesystem::exists(shaderPath)) {
            throw std::runtime_error("Shader file not found: " + shaderPath.string() +
                "\n\nPlease ensure the shader source (.vert/.frag/etc.) exists.");
        }

        if (!std::filesystem::is_regular_file(shaderPath)) {
            throw std::runtime_error("Path is not a regular file: " + shaderPath.string());
        }

        std::ifstream sourceFile(shaderPath);
        if (!sourceFile) {
            throw std::runtime_error("Failed to open shader source: " + shaderPath.string());
        }
        shaderSource.assign((std::istreambuf_iterator<char>(sourceFile)),
                            std::istreambuf_iterator<char>());
        sourceFile.close();
        shaderLabel = shaderPath.string();
    }

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);

    shaderc_shader_kind kind = ShadercKindFromStage(stage);
    auto result = compiler.CompileGlslToSpv(shaderSource, kind, shaderLabel.c_str(), options);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::string error = result.GetErrorMessage();
        logger_->Error("Shader compilation failed: " + shaderLabel + "\n" + error);
        throw std::runtime_error("Shader compilation failed: " + shaderLabel + "\n" + error);
    }

    std::vector<uint32_t> spirv(result.cbegin(), result.cend());
    std::vector<char> buffer(spirv.size() * sizeof(uint32_t));
    if (!buffer.empty()) {
        std::memcpy(buffer.data(), spirv.data(), buffer.size());
    }

    logger_->Debug("Compiled shader: " + shaderLabel +
                   " (" + std::to_string(buffer.size()) + " bytes)");

    auto inserted = shaderSpirvCache_.emplace(cacheKey, std::move(buffer));
    logger_->Trace("PipelineService", "ReadShaderSource",
                   "cacheHit=false, bytes=" + std::to_string(inserted.first->second.size()));
    return inserted.first->second;
}

}  // namespace sdl3cpp::services::impl
