#include "render_command_service.hpp"
#include "../../core/vertex.hpp"
#include <algorithm>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

RenderCommandService::RenderCommandService(std::shared_ptr<IVulkanDeviceService> deviceService,
                                           std::shared_ptr<ISwapchainService> swapchainService,
                                           std::shared_ptr<IPipelineService> pipelineService,
                                           std::shared_ptr<IBufferService> bufferService,
                                           std::shared_ptr<IGuiRendererService> guiRendererService,
                                           std::shared_ptr<JsonConfigService> configService,
                                           std::shared_ptr<ILogger> logger)
    : deviceService_(std::move(deviceService)),
      swapchainService_(std::move(swapchainService)),
      pipelineService_(std::move(pipelineService)),
      bufferService_(std::move(bufferService)),
      guiRendererService_(std::move(guiRendererService)),
      configService_(std::move(configService)),
      logger_(logger) {
    if (logger_) {
        logger_->Trace("RenderCommandService", "RenderCommandService",
                       "deviceService=" + std::string(deviceService_ ? "set" : "null") +
                       ", swapchainService=" + std::string(swapchainService_ ? "set" : "null") +
                       ", pipelineService=" + std::string(pipelineService_ ? "set" : "null") +
                       ", bufferService=" + std::string(bufferService_ ? "set" : "null") +
                       ", guiRendererService=" + std::string(guiRendererService_ ? "set" : "null"));
    }

    if (!deviceService_ || !swapchainService_ || !pipelineService_ || !bufferService_) {
        throw std::invalid_argument("All render command dependencies must be provided");
    }
}

RenderCommandService::~RenderCommandService() {
    if (logger_) {
        logger_->Trace("RenderCommandService", "~RenderCommandService");
    }
    if (commandPool_ != VK_NULL_HANDLE || imageAvailableSemaphore_ != VK_NULL_HANDLE) {
        Shutdown();
    }
}

void RenderCommandService::Cleanup() {
    logger_->Trace("RenderCommandService", "Cleanup");
    CleanupCommandResources();
    CleanupSyncObjects();
    CleanupRenderGraphResources();
    CleanupDescriptorResources();
}

void RenderCommandService::Shutdown() noexcept {
    logger_->Trace("RenderCommandService", "Shutdown");
    Cleanup();
}

bool RenderCommandService::BeginFrame(uint32_t& imageIndex) {
    logger_->Trace("RenderCommandService", "BeginFrame",
                   "imageIndex=" + std::to_string(imageIndex));

    // Lazy initialization
    if (commandPool_ == VK_NULL_HANDLE) {
        CreateCommandPool();
        CreateCommandBuffers();
        CreateSyncObjects();
        logger_->Info("RenderCommandService initialized lazily");
    }

    auto device = deviceService_->GetDevice();

    // Wait for previous frame (with timeout)
    constexpr uint64_t kFenceTimeout = 5000000000ULL; // 5 seconds
    VkResult fenceResult = vkWaitForFences(device, 1, &inFlightFence_, VK_TRUE, kFenceTimeout);

    if (fenceResult == VK_TIMEOUT) {
        logger_->Error("Fence wait timeout: GPU appears to be hung");
        throw std::runtime_error("Fence wait timeout: GPU appears to be hung");
    } else if (fenceResult != VK_SUCCESS) {
        logger_->Error("Fence wait failed");
        throw std::runtime_error("Fence wait failed");
    }

    vkResetFences(device, 1, &inFlightFence_);

    // Acquire next image
    VkResult result = swapchainService_->AcquireNextImage(imageAvailableSemaphore_, imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return false;  // Need swapchain recreation
    } else if (result == VK_TIMEOUT) {
        logger_->Error("Image acquisition timeout");
        throw std::runtime_error("Image acquisition timeout: GPU appears to be hung");
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to acquire swap chain image");
    }

    return true;
}

void RenderCommandService::RecordCommands(uint32_t imageIndex,
                                         const std::vector<RenderCommand>& commands,
                                         const std::array<float, 16>& viewProj) {
    logger_->Trace("RenderCommandService", "RecordCommands",
                   "imageIndex=" + std::to_string(imageIndex) +
                   ", commands.size=" + std::to_string(commands.size()) +
                   ", viewProj.size=" + std::to_string(viewProj.size()));

    VkCommandBuffer commandBuffer = commandBuffers_[imageIndex];

    // Reset command buffer
    vkResetCommandBuffer(commandBuffer, 0);

    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    EnsureDummyImageLayout(commandBuffer);

    EnsureDescriptorResources();
    EnsureDummyImageLayout(commandBuffer);

    // Begin render pass
    auto framebuffers = swapchainService_->GetSwapchainFramebuffers();
    auto extent = swapchainService_->GetSwapchainExtent();
    auto renderPass = swapchainService_->GetRenderPass();

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = extent;

    const auto& config = configService_->GetConfig();
    std::array<VkClearValue, 2> clearValues{};
    const auto& skyColor = config.atmospherics.skyColor;
    clearValues[0].color = {{skyColor[0], skyColor[1], skyColor[2], 1.0f}};  // Skybox clear
    clearValues[1].depthStencil = {1.0f, 0};  // Depth clear

    if (logger_) {
        logger_->Trace("RenderCommandService", "RecordCommands",
                       "clearColor=" + std::to_string(skyColor[0]) + "," +
                       std::to_string(skyColor[1]) + "," +
                       std::to_string(skyColor[2]));
    }

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    if (commands.empty()) {
        logger_->Trace("RenderCommandService", "RecordCommands",
                       "No render commands to draw; skipping buffer bind");
    } else {
        VkBuffer vertexBuffer = bufferService_->GetVertexBuffer();
        VkBuffer indexBuffer = bufferService_->GetIndexBuffer();
        if (vertexBuffer == VK_NULL_HANDLE || indexBuffer == VK_NULL_HANDLE) {
            logger_->Error("RenderCommandService: Vertex or index buffer not initialized");
        } else {
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

            VkPipelineLayout pipelineLayout = pipelineService_->GetPipelineLayout();
            if (pipelineLayout == VK_NULL_HANDLE) {
                logger_->Error("RenderCommandService: Pipeline layout is not initialized");
            } else {
                if (defaultDescriptorSet_ == VK_NULL_HANDLE) {
                    logger_->Error("RenderCommandService: Default descriptor set not available");
                    vkCmdEndRenderPass(commandBuffer);
                    vkEndCommandBuffer(commandBuffer);
                    return;
                }
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        pipelineLayout, 0, 1, &defaultDescriptorSet_, 0, nullptr);
                if (logger_) {
                    logger_->Trace("RenderCommandService", "RecordCommands",
                                   "drawing commands=" + std::to_string(commands.size()));
                }
                size_t drawIndex = 0;
                for (const auto& command : commands) {
                    if (!pipelineService_->HasShader(command.shaderKey)) {
                        logger_->Error("RenderCommandService: Missing pipeline for shader key: " + command.shaderKey);
                        continue;
                    }

                    if (logger_) {
                        logger_->Trace("RenderCommandService", "RecordCommands",
                                       "draw=" + std::to_string(drawIndex) +
                                       ", shaderKey=" + command.shaderKey +
                                       ", indexOffset=" + std::to_string(command.indexOffset) +
                                       ", indexCount=" + std::to_string(command.indexCount) +
                                       ", vertexOffset=" + std::to_string(command.vertexOffset));
                    }

                    VkPipeline pipeline = pipelineService_->GetPipeline(command.shaderKey);
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                    core::PushConstants pushConstants{};
                    pushConstants.model = command.modelMatrix;
                    pushConstants.viewProj = viewProj;

                    // For PBR shaders, populate extended push constants
                    if (command.shaderKey.find("pbr") != std::string::npos) {
                        // For now, use identity for view and proj (since viewProj is already combined)
                        // In a full implementation, we'd need separate view/proj matrices
                        pushConstants.view = {1.0f, 0.0f, 0.0f, 0.0f,
                                             0.0f, 1.0f, 0.0f, 0.0f,
                                             0.0f, 0.0f, 1.0f, 0.0f,
                                             0.0f, 0.0f, 0.0f, 1.0f};
                        pushConstants.proj = pushConstants.view; // Identity for now
                        pushConstants.lightViewProj = pushConstants.view; // Identity for now

                        // Camera position (0,0,0) for now - would need to be passed from scene
                        pushConstants.cameraPos = {0.0f, 0.0f, 0.0f};
                        pushConstants.time = 0.0f; // Would need actual time

                        // Atmospherics
                        pushConstants.ambientStrength = config.atmospherics.ambientStrength;
                        pushConstants.fogDensity = config.atmospherics.fogDensity;
                        pushConstants.fogStart = 0.0f;
                        pushConstants.fogEnd = 100.0f;
                        pushConstants.fogColor = config.atmospherics.fogColor;
                        pushConstants.gamma = config.atmospherics.gamma;
                        pushConstants.exposure = config.atmospherics.exposure;
                        pushConstants.enableShadows = config.atmospherics.enableShadows ? 1 : 0;
                        pushConstants.enableFog = 1; // Enable fog for PBR
                    }

                    vkCmdPushConstants(commandBuffer, pipelineLayout,
                                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                       sizeof(core::PushConstants), &pushConstants);

                    vkCmdDrawIndexed(commandBuffer, command.indexCount, 1, command.indexOffset, command.vertexOffset, 0);
                    ++drawIndex;
                }
            }
        }
    }

    // Render GUI overlay BEFORE ending render pass so we can use alpha blending
    if (guiRendererService_) {
        bool guiReady = guiRendererService_->IsReady();
        const auto& images = swapchainService_->GetSwapchainImages();
        if (!guiReady) {
            if (logger_) {
                logger_->Trace("RenderCommandService", "RecordCommands",
                               "GUI overlay skipped: renderer not ready");
            }
        } else if (imageIndex >= images.size() || images[imageIndex] == VK_NULL_HANDLE) {
            if (logger_) {
                logger_->Error("RenderCommandService: GUI overlay skipped due to invalid swapchain image");
            }
        } else {
            if (logger_) {
                logger_->Trace("RenderCommandService", "RecordCommands",
                               "Rendering GUI overlay imageIndex=" + std::to_string(imageIndex));
            }
            guiRendererService_->RenderToSwapchain(commandBuffer, images[imageIndex]);
        }
    } else if (logger_) {
        logger_->Trace("RenderCommandService", "RecordCommands",
                       "GUI overlay skipped: renderer service not available");
    }

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);
}

void RenderCommandService::RecordRenderGraph(uint32_t imageIndex,
                                             const RenderGraphDefinition& graph,
                                             const std::vector<RenderCommand>& commands,
                                             const std::array<float, 16>& viewProj) {
    logger_->Trace("RenderCommandService", "RecordRenderGraph",
                   "imageIndex=" + std::to_string(imageIndex) +
                   ", commands.size=" + std::to_string(commands.size()) +
                   ", resources=" + std::to_string(graph.resources.size()) +
                   ", passes=" + std::to_string(graph.passes.size()));

    EnsureRenderGraphResources(graph);
    EnsureDescriptorResources();

    VkCommandBuffer commandBuffer = commandBuffers_[imageIndex];
    vkResetCommandBuffer(commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkPipelineLayout pipelineLayout = pipelineService_->GetPipelineLayout();
    if (pipelineLayout == VK_NULL_HANDLE) {
        logger_->Error("RenderCommandService: Pipeline layout is not initialized");
        vkEndCommandBuffer(commandBuffer);
        return;
    }

    VkBuffer vertexBuffer = bufferService_->GetVertexBuffer();
    VkBuffer indexBuffer = bufferService_->GetIndexBuffer();
    bool sceneBuffersReady = vertexBuffer != VK_NULL_HANDLE && indexBuffer != VK_NULL_HANDLE;
    if (sceneBuffersReady) {
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    } else {
        logger_->Warn("RenderCommandService: Vertex or index buffer not initialized for render graph");
    }

    const auto& config = configService_->GetConfig();
    const auto& skyColor = config.atmospherics.skyColor;

    std::array<VkClearValue, 2> sceneClear{};
    sceneClear[0].color = {{skyColor[0], skyColor[1], skyColor[2], 1.0f}};
    sceneClear[1].depthStencil = {1.0f, 0};

    std::array<VkClearValue, 2> postClear{};
    postClear[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    postClear[1].depthStencil = {1.0f, 0};

    auto framebuffers = swapchainService_->GetSwapchainFramebuffers();
    auto swapchainRenderPass = swapchainService_->GetRenderPass();
    auto extent = swapchainService_->GetSwapchainExtent();

    bool swapchainUsed = false;

    for (const auto& pass : graph.passes) {
        if (!IsPassEnabled(pass)) {
            if (logger_) {
                logger_->Trace("RenderCommandService", "RecordRenderGraph",
                               "Skipping disabled pass=" + pass.name);
            }
            continue;
        }
        if (pass.shader.empty()) {
            if (logger_) {
                logger_->Warn("RenderCommandService: Render graph pass '" + pass.name + "' has no shader");
            }
            continue;
        }

        std::string outputName = ResolvePassOutput(pass);
        if (outputName.empty()) {
            if (logger_) {
                logger_->Warn("RenderCommandService: Render graph pass '" + pass.name + "' has no output");
            }
            continue;
        }

        bool outputSwapchain = outputName == "swapchain";
        VkRenderPass renderPass = outputSwapchain ? swapchainRenderPass : offscreenRenderPass_;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        VkExtent2D targetExtent = extent;
        RenderGraphImage* target = nullptr;

        if (!outputSwapchain && renderPass == VK_NULL_HANDLE) {
            if (logger_) {
                logger_->Warn("RenderCommandService: Offscreen render pass is not initialized");
            }
            continue;
        }

        if (outputSwapchain) {
            if (imageIndex >= framebuffers.size()) {
                logger_->Error("RenderCommandService: Swapchain framebuffer index out of range");
                continue;
            }
            framebuffer = framebuffers[imageIndex];
            targetExtent = extent;
            swapchainUsed = true;
        } else {
            target = FindRenderTarget(outputName);
            if (!target || target->framebuffer == VK_NULL_HANDLE) {
                if (logger_) {
                    logger_->Warn("RenderCommandService: Render target not found for '" + outputName + "'");
                }
                continue;
            }
            framebuffer = target->framebuffer;
            targetExtent = target->extent;
            TransitionImageLayout(commandBuffer, *target, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                  VK_IMAGE_ASPECT_COLOR_BIT);
            RenderGraphImage* depthTarget = FindDepthTarget(outputName);
            if (depthTarget) {
                TransitionImageLayout(commandBuffer, *depthTarget,
                                      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                      VK_IMAGE_ASPECT_DEPTH_BIT);
            } else if (logger_) {
                logger_->Warn("RenderCommandService: Depth target not found for '" + outputName + "'");
            }
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffer;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = targetExtent;

        const auto& clearValues = IsScenePass(pass) ? sceneClear : postClear;
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        bool scenePass = IsScenePass(pass);
        if (!scenePass) {
            if (!pipelineService_->HasShader(pass.shader)) {
                if (logger_) {
                    logger_->Warn("RenderCommandService: Missing pipeline for shader key: " + pass.shader);
                }
                vkCmdEndRenderPass(commandBuffer);
                continue;
            }

            VkPipeline pipeline = pipelineService_->GetPipeline(pass.shader);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        }

        if (scenePass) {
            if (defaultDescriptorSet_ == VK_NULL_HANDLE) {
                if (logger_) {
                    logger_->Warn("RenderCommandService: Default descriptor set not available");
                }
                vkCmdEndRenderPass(commandBuffer);
                continue;
            }
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pipelineLayout, 0, 1, &defaultDescriptorSet_, 0, nullptr);

            if (!sceneBuffersReady) {
                if (logger_) {
                    logger_->Warn("RenderCommandService: Skipping scene pass due to missing buffers");
                }
            } else if (commands.empty()) {
                if (logger_) {
                    logger_->Trace("RenderCommandService", "RecordRenderGraph",
                                   "No scene commands for pass=" + pass.name);
                }
            } else {
                size_t drawIndex = 0;
                for (const auto& command : commands) {
                    if (!pipelineService_->HasShader(command.shaderKey)) {
                        logger_->Error("RenderCommandService: Missing pipeline for shader key: " + command.shaderKey);
                        continue;
                    }

                    if (logger_) {
                        logger_->Trace("RenderCommandService", "RecordRenderGraph",
                                       "pass=" + pass.name +
                                       ", draw=" + std::to_string(drawIndex) +
                                       ", shaderKey=" + command.shaderKey);
                    }

                    VkPipeline scenePipeline = pipelineService_->GetPipeline(command.shaderKey);
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scenePipeline);

                    core::PushConstants pushConstants{};
                    pushConstants.model = command.modelMatrix;
                    pushConstants.viewProj = viewProj;

                    if (command.shaderKey.find("pbr") != std::string::npos) {
                        pushConstants.view = {1.0f, 0.0f, 0.0f, 0.0f,
                                             0.0f, 1.0f, 0.0f, 0.0f,
                                             0.0f, 0.0f, 1.0f, 0.0f,
                                             0.0f, 0.0f, 0.0f, 1.0f};
                        pushConstants.proj = pushConstants.view;
                        pushConstants.lightViewProj = pushConstants.view;
                        pushConstants.cameraPos = {0.0f, 0.0f, 0.0f};
                        pushConstants.time = 0.0f;
                        pushConstants.ambientStrength = config.atmospherics.ambientStrength;
                        pushConstants.fogDensity = config.atmospherics.fogDensity;
                        pushConstants.fogStart = 0.0f;
                        pushConstants.fogEnd = 100.0f;
                        pushConstants.fogColor = config.atmospherics.fogColor;
                        pushConstants.gamma = config.atmospherics.gamma;
                        pushConstants.exposure = config.atmospherics.exposure;
                        pushConstants.enableShadows = config.atmospherics.enableShadows ? 1 : 0;
                        pushConstants.enableFog = 1;
                    }

                    vkCmdPushConstants(commandBuffer, pipelineLayout,
                                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                       sizeof(core::PushConstants), &pushConstants);
                    vkCmdDrawIndexed(commandBuffer, command.indexCount, 1,
                                     command.indexOffset, command.vertexOffset, 0);
                    ++drawIndex;
                }
            }
        } else {
            std::string inputName = ResolvePassInput(pass);
            if (inputName == "swapchain" && outputSwapchain) {
                if (logger_) {
                    logger_->Trace("RenderCommandService", "RecordRenderGraph",
                                   "Skipping feedback pass=" + pass.name);
                }
                vkCmdEndRenderPass(commandBuffer);
                continue;
            }

            bool hasInput = true;
            VkImageView inputView = VK_NULL_HANDLE;
            if (inputName.empty()) {
                hasInput = false;
                if (logger_) {
                    logger_->Warn("RenderCommandService: Pass '" + pass.name + "' missing input");
                }
            } else if (inputName == "swapchain") {
                hasInput = false;
                if (logger_) {
                    logger_->Warn("RenderCommandService: Pass '" + pass.name +
                                  "' cannot sample from swapchain output");
                }
            } else {
                RenderGraphImage* inputTarget = FindRenderTarget(inputName);
                if (inputTarget) {
                    TransitionImageLayout(commandBuffer, *inputTarget, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                          VK_IMAGE_ASPECT_COLOR_BIT);
                    inputView = inputTarget->view;
                } else {
                    hasInput = false;
                    if (logger_) {
                        logger_->Warn("RenderCommandService: Input target not found for '" + inputName + "'");
                    }
                }
            }

            if (!hasInput || inputView == VK_NULL_HANDLE) {
                vkCmdEndRenderPass(commandBuffer);
                continue;
            }

            if (postProcessDescriptorSet_ == VK_NULL_HANDLE) {
                if (logger_) {
                    logger_->Warn("RenderCommandService: Post-process descriptor set not available");
                }
                vkCmdEndRenderPass(commandBuffer);
                continue;
            }

            UpdateDescriptorSet(postProcessDescriptorSet_, inputView);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pipelineLayout, 0, 1, &postProcessDescriptorSet_, 0, nullptr);

            core::PushConstants pushConstants = BuildFullscreenConstants(pass);
            vkCmdPushConstants(commandBuffer, pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                               sizeof(core::PushConstants), &pushConstants);
            vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        }

        if (outputSwapchain && guiRendererService_) {
            const auto& images = swapchainService_->GetSwapchainImages();
            if (imageIndex < images.size() && images[imageIndex] != VK_NULL_HANDLE) {
                guiRendererService_->RenderToSwapchain(commandBuffer, images[imageIndex]);
            }
        }

        vkCmdEndRenderPass(commandBuffer);

        if (!outputSwapchain && target) {
            TransitionImageLayout(commandBuffer, *target, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                  VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    if (!swapchainUsed && logger_) {
        logger_->Warn("RenderCommandService: Render graph did not output to swapchain");
    }

    vkEndCommandBuffer(commandBuffer);
}

bool RenderCommandService::EndFrame(uint32_t imageIndex) {
    logger_->Trace("RenderCommandService", "EndFrame",
                   "imageIndex=" + std::to_string(imageIndex));

    auto device = deviceService_->GetDevice();
    auto graphicsQueue = deviceService_->GetGraphicsQueue();

    VkCommandBuffer commandBuffer = commandBuffers_[imageIndex];

    // Submit command buffer
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore_};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore_};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer");
    }

    // Present
    std::vector<VkSemaphore> presentWaitSemaphores = {renderFinishedSemaphore_};
    VkResult result = swapchainService_->Present(presentWaitSemaphores, imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return false;  // Need swapchain recreation
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image");
    }

    return true;
}

VkCommandBuffer RenderCommandService::GetCurrentCommandBuffer() const {
    logger_->Trace("RenderCommandService", "GetCurrentCommandBuffer");
    if (commandBuffers_.empty()) {
        return VK_NULL_HANDLE;
    }
    return commandBuffers_[currentFrame_];
}

void RenderCommandService::OnSwapchainRecreated() {
    logger_->Trace("RenderCommandService", "OnSwapchainRecreated");

    Cleanup();
    currentFrame_ = 0;

    if (guiRendererService_) {
        VkExtent2D extent = swapchainService_->GetSwapchainExtent();
        VkFormat format = swapchainService_->GetSwapchainImageFormat();
        VkRenderPass renderPass = swapchainService_->GetRenderPass();
        guiRendererService_->Resize(extent.width, extent.height, format, renderPass);
    }
}

void RenderCommandService::CreateCommandPool() {
    logger_->Trace("RenderCommandService", "CreateCommandPool");

    auto device = deviceService_->GetDevice();
    auto queueFamilies = deviceService_->GetQueueFamilies();

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilies.graphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }
}

void RenderCommandService::CreateCommandBuffers() {
    logger_->Trace("RenderCommandService", "CreateCommandBuffers");

    auto device = deviceService_->GetDevice();
    auto framebuffers = swapchainService_->GetSwapchainFramebuffers();

    commandBuffers_.resize(framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers_.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }

    logger_->Debug("Created " + std::to_string(commandBuffers_.size()) + " command buffers");
}

void RenderCommandService::CreateSyncObjects() {
    logger_->Trace("RenderCommandService", "CreateSyncObjects");

    auto device = deviceService_->GetDevice();

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore_) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore_) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create synchronization objects");
    }

    logger_->Debug("Created synchronization objects");
}

void RenderCommandService::CleanupCommandResources() {
    logger_->Trace("RenderCommandService", "CleanupCommandResources");

    auto device = deviceService_->GetDevice();

    if (!commandBuffers_.empty() && commandPool_ != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(device, commandPool_,
                            static_cast<uint32_t>(commandBuffers_.size()),
                            commandBuffers_.data());
        commandBuffers_.clear();
    }

    if (commandPool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, commandPool_, nullptr);
        commandPool_ = VK_NULL_HANDLE;
    }
}

void RenderCommandService::CleanupSyncObjects() {
    logger_->Trace("RenderCommandService", "CleanupSyncObjects");

    auto device = deviceService_->GetDevice();

    if (imageAvailableSemaphore_ != VK_NULL_HANDLE) {
        vkDestroySemaphore(device, imageAvailableSemaphore_, nullptr);
        imageAvailableSemaphore_ = VK_NULL_HANDLE;
    }

    if (renderFinishedSemaphore_ != VK_NULL_HANDLE) {
        vkDestroySemaphore(device, renderFinishedSemaphore_, nullptr);
        renderFinishedSemaphore_ = VK_NULL_HANDLE;
    }

    if (inFlightFence_ != VK_NULL_HANDLE) {
        vkDestroyFence(device, inFlightFence_, nullptr);
        inFlightFence_ = VK_NULL_HANDLE;
    }
}

void RenderCommandService::EnsureDescriptorResources() {
    if (descriptorPool_ != VK_NULL_HANDLE) {
        return;
    }
    if (logger_) {
        logger_->Trace("RenderCommandService", "EnsureDescriptorResources");
    }
    CreateDescriptorPool();
    CreateSampler();
    CreateDummyImage();
    AllocateDescriptorSets();
    UpdateDescriptorSet(defaultDescriptorSet_, dummyImageView_);
    UpdateDescriptorSet(postProcessDescriptorSet_, dummyImageView_);
}

void RenderCommandService::CleanupDescriptorResources() {
    logger_->Trace("RenderCommandService", "CleanupDescriptorResources");

    auto device = deviceService_->GetDevice();

    if (dummyImageView_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device, dummyImageView_, nullptr);
        dummyImageView_ = VK_NULL_HANDLE;
    }
    if (dummyImage_ != VK_NULL_HANDLE) {
        vkDestroyImage(device, dummyImage_, nullptr);
        dummyImage_ = VK_NULL_HANDLE;
    }
    if (dummyImageMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device, dummyImageMemory_, nullptr);
        dummyImageMemory_ = VK_NULL_HANDLE;
    }
    if (sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(device, sampler_, nullptr);
        sampler_ = VK_NULL_HANDLE;
    }
    if (descriptorPool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descriptorPool_, nullptr);
        descriptorPool_ = VK_NULL_HANDLE;
    }

    defaultDescriptorSet_ = VK_NULL_HANDLE;
    postProcessDescriptorSet_ = VK_NULL_HANDLE;
    dummyImageLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;
}

void RenderCommandService::CreateDescriptorPool() {
    logger_->Trace("RenderCommandService", "CreateDescriptorPool");

    auto device = deviceService_->GetDevice();

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 8;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 4;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool");
    }
}

void RenderCommandService::CreateSampler() {
    logger_->Trace("RenderCommandService", "CreateSampler");

    auto device = deviceService_->GetDevice();

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.maxLod = 1.0f;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create sampler");
    }
}

void RenderCommandService::CreateDummyImage() {
    logger_->Trace("RenderCommandService", "CreateDummyImage");

    auto device = deviceService_->GetDevice();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = 1;
    imageInfo.extent.height = 1;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &dummyImage_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create dummy image");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, dummyImage_, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = deviceService_->FindMemoryType(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &dummyImageMemory_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate dummy image memory");
    }

    vkBindImageMemory(device, dummyImage_, dummyImageMemory_, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = dummyImage_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &dummyImageView_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create dummy image view");
    }

    dummyImageLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;
}

void RenderCommandService::AllocateDescriptorSets() {
    logger_->Trace("RenderCommandService", "AllocateDescriptorSets");

    VkDescriptorSetLayout layout = pipelineService_->GetDescriptorSetLayout();
    if (layout == VK_NULL_HANDLE) {
        logger_->Error("RenderCommandService: Descriptor set layout is not initialized");
        return;
    }

    std::array<VkDescriptorSetLayout, 2> layouts = {layout, layout};
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();

    std::array<VkDescriptorSet, 2> sets{};
    if (vkAllocateDescriptorSets(deviceService_->GetDevice(), &allocInfo, sets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    defaultDescriptorSet_ = sets[0];
    postProcessDescriptorSet_ = sets[1];
}

void RenderCommandService::UpdateDescriptorSet(VkDescriptorSet set, VkImageView view) {
    if (set == VK_NULL_HANDLE || view == VK_NULL_HANDLE || sampler_ == VK_NULL_HANDLE) {
        return;
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = sampler_;
    imageInfo.imageView = view;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = set;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(deviceService_->GetDevice(), 1, &descriptorWrite, 0, nullptr);
}

void RenderCommandService::EnsureDummyImageLayout(VkCommandBuffer commandBuffer) {
    if (dummyImage_ == VK_NULL_HANDLE || dummyImageLayout_ == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        return;
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = dummyImageLayout_;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = dummyImage_;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    dummyImageLayout_ = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void RenderCommandService::EnsureRenderGraphResources(const RenderGraphDefinition& graph) {
    if (graph.resources.empty() || graph.passes.empty()) {
        return;
    }

    VkExtent2D extent = swapchainService_->GetSwapchainExtent();
    if (renderGraphResourceCount_ == graph.resources.size() &&
        renderGraphPassCount_ == graph.passes.size() &&
        renderGraphExtent_.width == extent.width &&
        renderGraphExtent_.height == extent.height) {
        return;
    }

    CleanupRenderGraphResources();
    RegisterRenderGraphShaders(graph);

    renderGraphResourceCount_ = graph.resources.size();
    renderGraphPassCount_ = graph.passes.size();
    renderGraphExtent_ = extent;

    VkFormat colorFormat = swapchainService_->GetSwapchainImageFormat();
    VkFormat depthFormat = swapchainService_->GetDepthFormat();

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = colorFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 1;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(deviceService_->GetDevice(), &renderPassInfo, nullptr, &offscreenRenderPass_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create offscreen render pass");
    }

    for (const auto& resource : graph.resources) {
        if (resource.type != "color" || resource.name == "swapchain") {
            continue;
        }
        if (resource.layers > 1 || resource.mips > 1) {
            if (logger_) {
                logger_->Warn("RenderCommandService: Skipping layered/mipped resource '" + resource.name + "'");
            }
            continue;
        }

        VkExtent2D targetExtent = ResolveExtent(resource);
        if (targetExtent.width != extent.width || targetExtent.height != extent.height) {
            if (logger_) {
                logger_->Warn("RenderCommandService: Skipping resource '" + resource.name +
                              "' due to unsupported extent");
            }
            continue;
        }
        VkFormat targetFormat = ResolveColorFormat(resource.format);
        if (targetFormat != colorFormat && logger_) {
            logger_->Trace("RenderCommandService", "EnsureRenderGraphResources",
                           "Resource '" + resource.name + "' format overridden to swapchain format");
        }

        RenderGraphImage color{};
        CreateRenderGraphImage(color, colorFormat, targetExtent,
                               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                               VK_IMAGE_ASPECT_COLOR_BIT);

        RenderGraphImage depth{};
        CreateRenderGraphImage(depth, depthFormat, targetExtent,
                               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                               VK_IMAGE_ASPECT_DEPTH_BIT);

        std::array<VkImageView, 2> framebufferAttachments = {color.view, depth.view};
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = offscreenRenderPass_;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(framebufferAttachments.size());
        framebufferInfo.pAttachments = framebufferAttachments.data();
        framebufferInfo.width = targetExtent.width;
        framebufferInfo.height = targetExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(deviceService_->GetDevice(), &framebufferInfo, nullptr, &color.framebuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render graph framebuffer for " + resource.name);
        }

        renderGraphTargets_.emplace(resource.name, color);
        renderGraphDepth_.emplace(resource.name, depth);
    }
}

void RenderCommandService::CleanupRenderGraphResources() {
    logger_->Trace("RenderCommandService", "CleanupRenderGraphResources");

    auto device = deviceService_->GetDevice();

    for (auto& [name, image] : renderGraphTargets_) {
        if (image.framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, image.framebuffer, nullptr);
            image.framebuffer = VK_NULL_HANDLE;
        }
        if (image.view != VK_NULL_HANDLE) {
            vkDestroyImageView(device, image.view, nullptr);
            image.view = VK_NULL_HANDLE;
        }
        if (image.image != VK_NULL_HANDLE) {
            vkDestroyImage(device, image.image, nullptr);
            image.image = VK_NULL_HANDLE;
        }
        if (image.memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, image.memory, nullptr);
            image.memory = VK_NULL_HANDLE;
        }
    }
    renderGraphTargets_.clear();

    for (auto& [name, image] : renderGraphDepth_) {
        if (image.view != VK_NULL_HANDLE) {
            vkDestroyImageView(device, image.view, nullptr);
            image.view = VK_NULL_HANDLE;
        }
        if (image.image != VK_NULL_HANDLE) {
            vkDestroyImage(device, image.image, nullptr);
            image.image = VK_NULL_HANDLE;
        }
        if (image.memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, image.memory, nullptr);
            image.memory = VK_NULL_HANDLE;
        }
    }
    renderGraphDepth_.clear();

    if (offscreenRenderPass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, offscreenRenderPass_, nullptr);
        offscreenRenderPass_ = VK_NULL_HANDLE;
    }

    renderGraphResourceCount_ = 0;
    renderGraphPassCount_ = 0;
    renderGraphExtent_ = {};
}

void RenderCommandService::RegisterRenderGraphShaders(const RenderGraphDefinition& graph) {
    static const char* kFullscreenVertex = R"(
#version 450

layout(location = 0) out vec2 fragTexCoord;

vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2(3.0, -1.0),
    vec2(-1.0, 3.0)
);

vec2 uvs[3] = vec2[](
    vec2(0.0, 0.0),
    vec2(2.0, 0.0),
    vec2(0.0, 2.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragTexCoord = uvs[gl_VertexIndex];
}
)";

    static const char* kPostProcessFragment = R"(
#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputTexture;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProj;
    mat4 view;
    mat4 proj;
    mat4 lightViewProj;
    vec3 cameraPos;
    float time;
    float ambientStrength;
    float fogDensity;
    float fogStart;
    float fogEnd;
    vec3 fogColor;
    float gamma;
    float exposure;
    int enableShadows;
    int enableFog;
} pc;

void main() {
    vec3 color = texture(inputTexture, fragTexCoord).rgb;
    outColor = vec4(color, 1.0);
}
)";

    static const char* kTonemapFragment = R"(
#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputTexture;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProj;
    mat4 view;
    mat4 proj;
    mat4 lightViewProj;
    vec3 cameraPos;
    float time;
    float ambientStrength;
    float fogDensity;
    float fogStart;
    float fogEnd;
    vec3 fogColor;
    float gamma;
    float exposure;
    int enableShadows;
    int enableFog;
} pc;

void main() {
    vec3 color = texture(inputTexture, fragTexCoord).rgb;
    color *= pc.exposure;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / max(pc.gamma, 0.01)));
    outColor = vec4(color, 1.0);
}
)";

    bool registered = false;
    for (const auto& pass : graph.passes) {
        if (pass.shader.empty()) {
            continue;
        }
        if (IsScenePass(pass)) {
            continue;
        }
        if (pipelineService_->HasShader(pass.shader)) {
            continue;
        }

        ShaderPaths paths{};
        paths.vertexSource = kFullscreenVertex;
        if (pass.shader == "tonemap") {
            paths.fragmentSource = kTonemapFragment;
        } else {
            paths.fragmentSource = kPostProcessFragment;
        }
        pipelineService_->RegisterShader(pass.shader, paths);
        registered = true;
    }

    if (registered) {
        VkRenderPass renderPass = swapchainService_->GetRenderPass();
        VkExtent2D extent = swapchainService_->GetSwapchainExtent();
        pipelineService_->RecreatePipelines(renderPass, extent);
        CleanupDescriptorResources();
    }
}

VkFormat RenderCommandService::ResolveColorFormat(const std::string& format) const {
    (void)format;
    return swapchainService_->GetSwapchainImageFormat();
}

VkFormat RenderCommandService::ResolveDepthFormat(const std::string& format) const {
    (void)format;
    return swapchainService_->GetDepthFormat();
}

VkExtent2D RenderCommandService::ResolveExtent(const RenderGraphResource& resource) const {
    VkExtent2D swapchainExtent = swapchainService_->GetSwapchainExtent();
    if (resource.hasExplicitSize) {
        return {std::max(1u, resource.explicitSize[0]),
                std::max(1u, resource.explicitSize[1])};
    }
    if (resource.size == "half") {
        return {std::max(1u, swapchainExtent.width / 2),
                std::max(1u, swapchainExtent.height / 2)};
    }
    return swapchainExtent;
}

RenderGraphImage* RenderCommandService::FindRenderTarget(const std::string& name) {
    auto it = renderGraphTargets_.find(name);
    if (it == renderGraphTargets_.end()) {
        return nullptr;
    }
    return &it->second;
}

RenderGraphImage* RenderCommandService::FindDepthTarget(const std::string& name) {
    auto it = renderGraphDepth_.find(name);
    if (it == renderGraphDepth_.end()) {
        return nullptr;
    }
    return &it->second;
}

void RenderCommandService::CreateRenderGraphImage(RenderGraphImage& image, VkFormat format,
                                                  VkExtent2D extent, VkImageUsageFlags usage,
                                                  VkImageAspectFlags aspectMask) {
    auto device = deviceService_->GetDevice();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = extent.width;
    imageInfo.extent.height = extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image.image) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render graph image");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image.image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = deviceService_->FindMemoryType(
        memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &image.memory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate render graph image memory");
    }

    vkBindImageMemory(device, image.image, image.memory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &image.view) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render graph image view");
    }

    image.format = format;
    image.extent = extent;
    image.layout = VK_IMAGE_LAYOUT_UNDEFINED;
}

void RenderCommandService::TransitionImageLayout(VkCommandBuffer commandBuffer, RenderGraphImage& image,
                                                 VkImageLayout newLayout, VkImageAspectFlags aspectMask) {
    if (image.layout == newLayout || image.image == VK_NULL_HANDLE) {
        return;
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = image.layout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image.image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    if (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = (image.layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            ? VK_ACCESS_SHADER_READ_BIT : 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        srcStage = (image.layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    image.layout = newLayout;
}

bool RenderCommandService::IsScenePass(const RenderGraphPass& pass) const {
    return pass.kind == "gbuffer" || pass.kind == "forward_plus" || pass.kind == "transparent";
}

bool RenderCommandService::IsPassEnabled(const RenderGraphPass& pass) const {
    auto it = pass.settings.find("enabled");
    if (it != pass.settings.end() && it->second.type == RenderGraphValue::Type::Boolean) {
        return it->second.boolean;
    }
    return true;
}

std::string RenderCommandService::ResolvePassOutput(const RenderGraphPass& pass) const {
    if (!pass.output.empty()) {
        return pass.output;
    }
    if (!pass.outputs.empty()) {
        auto sceneIt = pass.outputs.find("scene");
        if (sceneIt != pass.outputs.end()) {
            return sceneIt->second;
        }
        auto albedoIt = pass.outputs.find("albedo");
        if (albedoIt != pass.outputs.end()) {
            return albedoIt->second;
        }
        auto colorIt = pass.outputs.find("color");
        if (colorIt != pass.outputs.end()) {
            return colorIt->second;
        }
        return pass.outputs.begin()->second;
    }
    return {};
}

std::string RenderCommandService::ResolvePassInput(const RenderGraphPass& pass) const {
    if (!pass.inputs.empty()) {
        auto sceneIt = pass.inputs.find("scene");
        if (sceneIt != pass.inputs.end()) {
            return sceneIt->second;
        }
        auto inputIt = pass.inputs.find("input");
        if (inputIt != pass.inputs.end()) {
            return inputIt->second;
        }
        auto albedoIt = pass.inputs.find("albedo");
        if (albedoIt != pass.inputs.end()) {
            return albedoIt->second;
        }
        auto colorIt = pass.inputs.find("color");
        if (colorIt != pass.inputs.end()) {
            return colorIt->second;
        }
        return pass.inputs.begin()->second;
    }
    return {};
}

core::PushConstants RenderCommandService::BuildFullscreenConstants(const RenderGraphPass& pass) const {
    core::PushConstants constants{};
    const std::array<float, 16> identity = {1.0f, 0.0f, 0.0f, 0.0f,
                                            0.0f, 1.0f, 0.0f, 0.0f,
                                            0.0f, 0.0f, 1.0f, 0.0f,
                                            0.0f, 0.0f, 0.0f, 1.0f};
    constants.model = identity;
    constants.viewProj = identity;
    constants.view = identity;
    constants.proj = identity;
    constants.lightViewProj = identity;
    constants.cameraPos = {0.0f, 0.0f, 0.0f};

    const auto& config = configService_->GetConfig();
    constants.gamma = config.atmospherics.gamma;
    constants.exposure = config.atmospherics.exposure;
    constants.ambientStrength = config.atmospherics.ambientStrength;
    constants.fogDensity = config.atmospherics.fogDensity;
    constants.fogColor = config.atmospherics.fogColor;
    constants.enableFog = 0;
    constants.enableShadows = 0;

    auto applySetting = [&](const std::string& key, float& target) {
        auto it = pass.settings.find(key);
        if (it != pass.settings.end() && it->second.type == RenderGraphValue::Type::Number) {
            target = static_cast<float>(it->second.number);
        }
    };

    applySetting("gamma", constants.gamma);
    applySetting("exposure", constants.exposure);

    return constants;
}

}  // namespace sdl3cpp::services::impl
