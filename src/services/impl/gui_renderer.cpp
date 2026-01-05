#include "gui_renderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "font8x8_basic.h"

namespace sdl3cpp::services::impl {
namespace {

bool ExtractAttribute(const std::string& source, const char* name, std::string& outValue) {
    std::string key = name;
    size_t pos = source.find(key);
    while (pos != std::string::npos) {
        size_t eq = source.find('=', pos + key.size());
        if (eq == std::string::npos) {
            break;
        }
        size_t valueStart = eq + 1;
        while (valueStart < source.size() &&
               std::isspace(static_cast<unsigned char>(source[valueStart]))) {
            valueStart++;
        }
        if (valueStart >= source.size()) {
            break;
        }
        char quote = source[valueStart];
        if (quote != '\"' && quote != '\'') {
            break;
        }
        size_t valueEnd = source.find(quote, valueStart + 1);
        if (valueEnd == std::string::npos) {
            break;
        }
        outValue = source.substr(valueStart + 1, valueEnd - valueStart - 1);
        return true;
    }
    return false;
}

float ParseFloatValue(const std::string& text) {
    try {
        size_t idx = 0;
        return std::stof(text, &idx);
    } catch (...) {
        return 0.0f;
    }
}

GuiColor ParseColorString(const std::string& text, const GuiColor& fallback) {
    if (text.empty() || text[0] != '#') {
        return fallback;
    }
    try {
        if (text.size() == 7) {
            unsigned int rgb = std::stoul(text.substr(1), nullptr, 16);
            return {((rgb >> 16) & 0xFF) / 255.0f, ((rgb >> 8) & 0xFF) / 255.0f,
                    (rgb & 0xFF) / 255.0f, 1.0f};
        }
        if (text.size() == 9) {
            unsigned int rgba = std::stoul(text.substr(1), nullptr, 16);
            return {((rgba >> 24) & 0xFF) / 255.0f, ((rgba >> 16) & 0xFF) / 255.0f,
                    ((rgba >> 8) & 0xFF) / 255.0f, (rgba & 0xFF) / 255.0f};
        }
    } catch (...) {
    }
    return fallback;
}

ParsedSvg ParseSvgFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Failed to open SVG file: " + path.string());
    }
    std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    ParsedSvg result;
    std::string value;
    if (ExtractAttribute(data, "viewBox", value)) {
        float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
        std::sscanf(value.c_str(), "%f %f %f %f", &x, &y, &w, &h);
        if (w > 0.0f && h > 0.0f) {
            result.viewWidth = w;
            result.viewHeight = h;
        }
    }
    if (ExtractAttribute(data, "width", value)) {
        result.viewWidth = ParseFloatValue(value);
    }
    if (ExtractAttribute(data, "height", value)) {
        result.viewHeight = ParseFloatValue(value);
    }
    if (result.viewWidth <= 0.0f) {
        result.viewWidth = 128.0f;
    }
    if (result.viewHeight <= 0.0f) {
        result.viewHeight = 128.0f;
    }

    size_t search = 0;
    while (true) {
        size_t tagStart = data.find("<circle", search);
        if (tagStart == std::string::npos) {
            break;
        }
        size_t tagEnd = data.find('>', tagStart);
        if (tagEnd == std::string::npos) {
            break;
        }
        std::string tag = data.substr(tagStart, tagEnd - tagStart);
        SvgCircle circle;
        std::string attr;
        if (ExtractAttribute(tag, "cx", attr)) {
            circle.cx = ParseFloatValue(attr);
        }
        if (ExtractAttribute(tag, "cy", attr)) {
            circle.cy = ParseFloatValue(attr);
        }
        if (ExtractAttribute(tag, "r", attr)) {
            circle.r = ParseFloatValue(attr);
        }
        if (ExtractAttribute(tag, "fill", attr)) {
            circle.color = ParseColorString(attr, circle.color);
        }
        result.circles.push_back(circle);
        search = tagEnd + 1;
    }
    return result;
}

std::vector<uint8_t> ReadFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to read file: " + path.string());
    }
    size_t fileSize = file.tellg();
    std::vector<uint8_t> buffer(fileSize);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    return buffer;
}

} // namespace

GuiRenderer::GuiRenderer(VkDevice device, VkPhysicalDevice physicalDevice, VkFormat swapchainFormat,
                         VkRenderPass renderPass, const std::filesystem::path& scriptDirectory,
                         std::shared_ptr<IBufferService> bufferService)
    : device_(device),
      physicalDevice_(physicalDevice),
      swapchainFormat_(swapchainFormat),
      renderPass_(renderPass),
      scriptDirectory_(scriptDirectory),
      bufferService_(std::move(bufferService)) {
}

GuiRenderer::~GuiRenderer() {
    CleanupBuffers();
    CleanupPipeline();
}

bool GuiRenderer::IsReady() const {
    return pipeline_ != VK_NULL_HANDLE && !vertices_.empty();
}

void GuiRenderer::Prepare(const std::vector<GuiCommand>& commands, uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) {
        return;
    }

    viewportWidth_ = width;
    viewportHeight_ = height;

    // Create pipeline if needed
    if (pipeline_ == VK_NULL_HANDLE) {
        CreatePipeline(renderPass_, {width, height});
    }

    // Generate geometry from GUI commands
    GenerateGuiGeometry(commands, width, height);

    // Create/update vertex and index buffers
    if (!vertices_.empty() && !indices_.empty()) {
        CreateVertexAndIndexBuffers(vertices_.size(), indices_.size());
    }
}

void GuiRenderer::RenderToSwapchain(VkCommandBuffer commandBuffer, VkRenderPass renderPass) {
    if (!IsReady() || vertices_.empty() || indices_.empty()) {
        return;
    }

    // Bind pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

    // Bind vertex and index buffers
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer_, &offset);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer_, 0, VK_INDEX_TYPE_UINT32);

    // TODO: Set up push constants for orthographic projection matrix here
    // For now, we'll use identity matrices which assumes the shaders handle 2D coordinates

    // Draw
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices_.size()), 1, 0, 0, 0);
}

void GuiRenderer::Resize(uint32_t width, uint32_t height, VkFormat format) {
    if (width == viewportWidth_ && height == viewportHeight_ && format == swapchainFormat_) {
        return;
    }
    UpdateFormat(format);
    viewportWidth_ = width;
    viewportHeight_ = height;

    // Recreate pipeline for new viewport size
    if (pipeline_ != VK_NULL_HANDLE) {
        CleanupPipeline();
        CreatePipeline(renderPass_, {width, height});
    }
}

void GuiRenderer::GenerateGuiGeometry(const std::vector<GuiCommand>& commands, uint32_t width, uint32_t height) {
    vertices_.clear();
    indices_.clear();

    // Convert screen coordinates to NDC (-1 to 1)
    auto toNDC = [width, height](float x, float y) -> std::pair<float, float> {
        return {
            (x / static_cast<float>(width)) * 2.0f - 1.0f,
            (y / static_cast<float>(height)) * 2.0f - 1.0f
        };
    };

    for (const auto& cmd : commands) {
        if (cmd.type == GuiCommand::Type::Rect) {
            // Generate a quad (2 triangles) for the rectangle
            auto [x0, y0] = toNDC(cmd.rect.x, cmd.rect.y);
            auto [x1, y1] = toNDC(cmd.rect.x + cmd.rect.width, cmd.rect.y + cmd.rect.height);

            uint32_t baseIndex = static_cast<uint32_t>(vertices_.size());

            // Add 4 vertices for the quad
            vertices_.push_back({x0, y0, 0.0f, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a});
            vertices_.push_back({x1, y0, 0.0f, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a});
            vertices_.push_back({x1, y1, 0.0f, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a});
            vertices_.push_back({x0, y1, 0.0f, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a});

            // Add 6 indices for 2 triangles
            indices_.push_back(baseIndex + 0);
            indices_.push_back(baseIndex + 1);
            indices_.push_back(baseIndex + 2);
            indices_.push_back(baseIndex + 0);
            indices_.push_back(baseIndex + 2);
            indices_.push_back(baseIndex + 3);
        }
        // TODO: Implement Text, SVG, and other command types
        // For now, just render rectangles to get the pipeline working
    }
}

void GuiRenderer::CreatePipeline(VkRenderPass renderPass, VkExtent2D extent) {
    // Load shader modules
    auto vertShaderCode = ReadFile(scriptDirectory_.parent_path() / "shaders" / "gui_2d.vert.spv");
    auto fragShaderCode = ReadFile(scriptDirectory_.parent_path() / "shaders" / "gui_2d.frag.spv");

    VkShaderModuleCreateInfo vertModuleInfo{};
    vertModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertModuleInfo.codeSize = vertShaderCode.size();
    vertModuleInfo.pCode = reinterpret_cast<const uint32_t*>(vertShaderCode.data());

    if (vkCreateShaderModule(device_, &vertModuleInfo, nullptr, &vertShaderModule_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex shader module for GUI");
    }

    VkShaderModuleCreateInfo fragModuleInfo{};
    fragModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragModuleInfo.codeSize = fragShaderCode.size();
    fragModuleInfo.pCode = reinterpret_cast<const uint32_t*>(fragShaderCode.data());

    if (vkCreateShaderModule(device_, &fragModuleInfo, nullptr, &fragShaderModule_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create fragment shader module for GUI");
    }

    // Shader stages
    VkPipelineShaderStageCreateInfo vertStageInfo{};
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vertShaderModule_;
    vertStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageInfo{};
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = fragShaderModule_;
    fragStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

    // Vertex input - GuiVertex format
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(GuiVertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> attributeDescs{};
    // Position (vec3)
    attributeDescs[0].binding = 0;
    attributeDescs[0].location = 0;
    attributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescs[0].offset = offsetof(GuiVertex, x);

    // Color (vec4)
    attributeDescs[1].binding = 0;
    attributeDescs[1].location = 1;
    attributeDescs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescs[1].offset = offsetof(GuiVertex, r);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescs.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs.data();

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
    rasterizer.cullMode = VK_CULL_MODE_NONE;  // No culling for 2D GUI
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // **CRITICAL: Alpha blending for transparency**
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;  // Enable blending!
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Depth stencil - disable depth test for 2D GUI overlay
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // Push constants for transformation matrices
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 32;  // 2 mat4s

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create GUI pipeline layout");
    }

    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout_;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create GUI graphics pipeline");
    }
}

void GuiRenderer::CreateVertexAndIndexBuffers(size_t vertexCount, size_t indexCount) {
    // Clean up old buffers
    CleanupBuffers();

    if (vertexCount == 0 || indexCount == 0) {
        return;
    }

    // Create vertex buffer
    VkDeviceSize vertexBufferSize = sizeof(GuiVertex) * vertexCount;
    bufferService_->CreateBuffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertexBuffer_,
        vertexMemory_
    );

    // Upload vertex data
    void* vertexData;
    vkMapMemory(device_, vertexMemory_, 0, vertexBufferSize, 0, &vertexData);
    std::memcpy(vertexData, vertices_.data(), vertexBufferSize);
    vkUnmapMemory(device_, vertexMemory_);

    // Create index buffer
    VkDeviceSize indexBufferSize = sizeof(uint32_t) * indexCount;
    bufferService_->CreateBuffer(
        indexBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        indexBuffer_,
        indexMemory_
    );

    // Upload index data
    void* indexData;
    vkMapMemory(device_, indexMemory_, 0, indexBufferSize, 0, &indexData);
    std::memcpy(indexData, indices_.data(), indexBufferSize);
    vkUnmapMemory(device_, indexMemory_);
}

void GuiRenderer::CleanupPipeline() {
    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }
    if (pipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
    }
    if (vertShaderModule_ != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device_, vertShaderModule_, nullptr);
        vertShaderModule_ = VK_NULL_HANDLE;
    }
    if (fragShaderModule_ != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device_, fragShaderModule_, nullptr);
        fragShaderModule_ = VK_NULL_HANDLE;
    }
}

void GuiRenderer::CleanupBuffers() {
    if (vertexBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, vertexBuffer_, nullptr);
        vertexBuffer_ = VK_NULL_HANDLE;
    }
    if (vertexMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, vertexMemory_, nullptr);
        vertexMemory_ = VK_NULL_HANDLE;
    }
    if (indexBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, indexBuffer_, nullptr);
        indexBuffer_ = VK_NULL_HANDLE;
    }
    if (indexMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, indexMemory_, nullptr);
        indexMemory_ = VK_NULL_HANDLE;
    }
}

void GuiRenderer::UpdateFormat(VkFormat format) {
    if (swapchainFormat_ == format) {
        return;
    }
    swapchainFormat_ = format;
}

const ParsedSvg* GuiRenderer::LoadSvg(const std::string& relativePath) {
    auto it = svgCache_.find(relativePath);
    if (it != svgCache_.end()) {
        return &it->second;
    }
    std::filesystem::path path = scriptDirectory_ / relativePath;
    try {
        ParsedSvg parsed = ParseSvgFile(path);
        auto inserted = svgCache_.emplace(relativePath, std::move(parsed));
        return &inserted.first->second;
    } catch (...) {
        return nullptr;
    }
}

} // namespace sdl3cpp::services::impl
