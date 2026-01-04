#include "app/sdl3_app.hpp"
#include "logging/logger.hpp"

#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace {
const std::unordered_map<SDL_Keycode, std::string> kGuiKeyNames = {
    {SDLK_BACKSPACE, "backspace"},
    {SDLK_DELETE, "delete"},
    {SDLK_LEFT, "left"},
    {SDLK_RIGHT, "right"},
    {SDLK_HOME, "home"},
    {SDLK_END, "end"},
    {SDLK_RETURN, "enter"},
    {SDLK_UP, "up"},
    {SDLK_DOWN, "down"},
};
} // namespace

namespace sdl3cpp::app {

void Sdl3App::PrintGpuDiagnostics(const std::string& errorContext) {
    std::stringstream ss;
    ss << "\n========================================\n";
    ss << "GPU DIAGNOSTIC REPORT\n";
    ss << "========================================\n";
    ss << "Error Context: " << errorContext << "\n\n";
    
    // Device properties
    if (physicalDevice_ != VK_NULL_HANDLE) {
        VkPhysicalDeviceProperties deviceProps{};
        vkGetPhysicalDeviceProperties(physicalDevice_, &deviceProps);
        
        ss << "=== GPU Information ===\n";
        ss << "Device Name: " << deviceProps.deviceName << "\n";
        ss << "Driver Version: " << VK_API_VERSION_MAJOR(deviceProps.driverVersion) << "."
                  << VK_API_VERSION_MINOR(deviceProps.driverVersion) << "."
                  << VK_API_VERSION_PATCH(deviceProps.driverVersion) << "\n";
        ss << "API Version: " << VK_API_VERSION_MAJOR(deviceProps.apiVersion) << "."
                  << VK_API_VERSION_MINOR(deviceProps.apiVersion) << "."
                  << VK_API_VERSION_PATCH(deviceProps.apiVersion) << "\n";
        ss << "Vendor ID: 0x" << std::hex << deviceProps.vendorID << std::dec << "\n";
        ss << "Device ID: 0x" << std::hex << deviceProps.deviceID << std::dec << "\n";
        
        VkPhysicalDeviceMemoryProperties memProps{};
        vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProps);
        
        ss << "\n=== Memory Information ===\n";
        uint64_t totalVRAM = 0;
        for (uint32_t i = 0; i < memProps.memoryHeapCount; i++) {
            if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                totalVRAM += memProps.memoryHeaps[i].size;
            }
        }
        ss << "Total VRAM: " << (totalVRAM / 1024 / 1024) << " MB\n";
        
        // Memory heaps breakdown
        ss << "Memory Heaps (" << memProps.memoryHeapCount << "):\n";
        for (uint32_t i = 0; i < memProps.memoryHeapCount; i++) {
            ss << "  Heap " << i << ": " << (memProps.memoryHeaps[i].size / 1024 / 1024) << " MB";
            if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                ss << " (Device Local)";
            }
            ss << "\n";
        }
    }
    
    // Swapchain state
    ss << "\n=== Swapchain State ===\n";
    ss << "Extent: " << swapChainExtent_.width << "x" << swapChainExtent_.height << "\n";
    ss << "Image Count: " << swapChainImages_.size() << "\n";
    ss << "Format: " << swapChainImageFormat_ << "\n";
    ss << "Consecutive Recreations: " << consecutiveSwapchainRecreations_ << "\n";
    ss << "Framebuffer Resized Flag: " << (framebufferResized_ ? "true" : "false") << "\n";
    ss << "First Frame Completed: " << (firstFrameCompleted_ ? "true" : "false") << "\n";
    
    // Render objects
    ss << "\n=== Scene State ===\n";
    ss << "Render Objects: " << renderObjects_.size() << "\n";
    ss << "Vertices: " << vertices_.size() << "\n";
    ss << "Indices: " << indices_.size() << "\n";
    ss << "Pipelines: " << graphicsPipelines_.size() << "\n";
    ss << "GUI Renderer Active: " << (guiRenderer_ ? "true" : "false") << "\n";
    ss << "GUI Has Commands: " << (guiHasCommands_ ? "true" : "false") << "\n";
    
    // Check device features that might be related
    if (physicalDevice_ != VK_NULL_HANDLE) {
        VkPhysicalDeviceFeatures deviceFeatures{};
        vkGetPhysicalDeviceFeatures(physicalDevice_, &deviceFeatures);
        
        ss << "\n=== Relevant Device Features ===\n";
        ss << "Geometry Shader: " << (deviceFeatures.geometryShader ? "supported" : "not supported") << "\n";
        ss << "Tessellation Shader: " << (deviceFeatures.tessellationShader ? "supported" : "not supported") << "\n";
        ss << "Multi Draw Indirect: " << (deviceFeatures.multiDrawIndirect ? "supported" : "not supported") << "\n";
    }
    
    // Queue properties
    if (physicalDevice_ != VK_NULL_HANDLE) {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, queueFamilies.data());
        
        ss << "\n=== Queue Families ===\n";
        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            ss << "Family " << i << ": " << queueFamilies[i].queueCount << " queues, flags: 0x"
                      << std::hex << queueFamilies[i].queueFlags << std::dec << "\n";
        }
    }
    
    ss << "\n=== Possible Causes ===\n";
    ss << "1. GPU driver crash or hang - Check dmesg for GPU reset messages\n";
    ss << "2. Infinite loop in shader code - Review vertex/fragment shaders\n";
    ss << "3. Command buffer submission issue - Check synchronization\n";
    ss << "4. GPU overheating or hardware issue - Monitor GPU temperature\n";
    ss << "5. Driver bug - Try updating GPU drivers to latest version\n";
    ss << "6. Resource exhaustion - Check system memory and VRAM usage\n";
    
    ss << "\n=== Recommended Actions ===\n";
    ss << "1. Check system logs: dmesg | grep -i 'gpu\\|radeon\\|amdgpu'\n";
    ss << "2. Update GPU drivers: sudo dnf update mesa-vulkan-drivers\n";
    ss << "3. Verify GPU health: radeontop or similar monitoring tool\n";
    ss << "4. Check for driver messages: journalctl -k | grep -i amdgpu\n";
    ss << "5. Try with different Vulkan settings or validation layers\n";
    ss << "========================================\n";
    
    // sdl3cpp::logging::Logger::GetInstance().Error(ss.str());
    std::cerr << ss.str() << std::endl;
}

void Sdl3App::CreateCommandBuffers() {
    sdl3cpp::logging::TraceGuard trace(__PRETTY_FUNCTION__);;
    commandBuffers_.resize(swapChainFramebuffers_.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());

    if (vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }
}

void Sdl3App::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, float time,
                                   const std::array<float, 16>& viewProj) {
    sdl3cpp::logging::TraceGuard trace(__PRETTY_FUNCTION__);;
    sdl3cpp::logging::Logger::GetInstance().TraceVariable("imageIndex", imageIndex);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass_;
    renderPassInfo.framebuffer = swapChainFramebuffers_[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent_;

    VkClearValue clearColor = {{{0.1f, 0.1f, 0.15f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkBuffer vertexBuffers[] = {vertexBuffer_};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer_, 0, VK_INDEX_TYPE_UINT16);
    core::PushConstants pushConstants{};
    pushConstants.viewProj = viewProj;
    // Temporarily disable drawing to test if hang is caused by draw commands
    /*
    for (const auto& object : renderObjects_) {
        auto pipelineIt = graphicsPipelines_.find(object.shaderKey);
        if (pipelineIt == graphicsPipelines_.end()) {
            pipelineIt = graphicsPipelines_.find(defaultShaderKey_);
            if (pipelineIt == graphicsPipelines_.end()) {
                throw std::runtime_error("Missing pipeline for shader key");
            }
        }
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineIt->second);
        pushConstants.model = scriptEngine_.ComputeModelMatrix(object.computeModelMatrixRef, time);
        vkCmdPushConstants(commandBuffer, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(core::PushConstants),
                           &pushConstants);
        vkCmdDrawIndexed(commandBuffer, object.indexCount, 1, object.indexOffset, object.vertexOffset, 0);
    }
    */
    vkCmdEndRenderPass(commandBuffer);
    // Temporarily disable GUI rendering to test if it's causing the GPU hang
    /*
    if (guiRenderer_) {
        guiRenderer_->BlitToSwapchain(commandBuffer, swapChainImages_[imageIndex]);
    }
    */
    vkEndCommandBuffer(commandBuffer);
}

void Sdl3App::ProcessGuiEvent(const SDL_Event& event) {
    sdl3cpp::logging::TraceGuard trace(__PRETTY_FUNCTION__);;
    switch (event.type) {
        case SDL_EVENT_MOUSE_MOTION:
            guiInputSnapshot_.mouseX = static_cast<float>(event.motion.x);
            guiInputSnapshot_.mouseY = static_cast<float>(event.motion.y);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                guiInputSnapshot_.mouseDown = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
            }
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            guiInputSnapshot_.wheel += static_cast<float>(event.wheel.y);
            break;
        case SDL_EVENT_TEXT_INPUT:
            guiInputSnapshot_.textInput.append(event.text.text);
            break;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
            SDL_Keycode key = event.key.key;
            auto it = kGuiKeyNames.find(key);
            if (it != kGuiKeyNames.end()) {
                guiInputSnapshot_.keyStates[it->second] = (event.type == SDL_EVENT_KEY_DOWN);
            }
            break;
        }
        default:
            break;
    }
}

void Sdl3App::SetupGuiRenderer() {
    sdl3cpp::logging::TraceGuard trace(__PRETTY_FUNCTION__);;
    guiHasCommands_ = scriptEngine_.HasGuiCommands();
    if (!guiHasCommands_) {
        guiRenderer_.reset();
        return;
    }
    if (!guiRenderer_) {
        guiRenderer_ =
            std::make_unique<gui::GuiRenderer>(device_, physicalDevice_, swapChainImageFormat_,
                                                scriptEngine_.GetScriptDirectory());
    }
    guiRenderer_->Resize(swapChainExtent_.width, swapChainExtent_.height, swapChainImageFormat_);
}

void Sdl3App::DrawFrame(float time) {
    sdl3cpp::logging::TraceGuard trace(__PRETTY_FUNCTION__);;
    sdl3cpp::logging::Logger::GetInstance().Debug("Drawing frame at time " + std::to_string(time));
    
    // Use reasonable timeout instead of infinite wait (5 seconds)
    constexpr uint64_t kFenceTimeout = 5000000000ULL; // 5 seconds in nanoseconds
    VkResult fenceResult = vkWaitForFences(device_, 1, &inFlightFence_, VK_TRUE, kFenceTimeout);
    if (fenceResult == VK_TIMEOUT) {
        sdl3cpp::logging::Logger::GetInstance().Error("Fence wait timeout: GPU appears to be hung");
        PrintGpuDiagnostics("Fence wait timeout after 5 seconds");
        throw std::runtime_error("Fence wait timeout: GPU appears to be hung");
    } else if (fenceResult != VK_SUCCESS) {
        sdl3cpp::logging::Logger::GetInstance().Error("Fence wait failed with code: " + std::to_string(fenceResult));
        PrintGpuDiagnostics("Fence wait failed with error code " + std::to_string(fenceResult));
        throw std::runtime_error("Fence wait failed");
    }
    vkResetFences(device_, 1, &inFlightFence_);

    uint32_t imageIndex;
    // Use reasonable timeout for image acquisition (5 seconds)
    VkResult result = vkAcquireNextImageKHR(device_, swapChain_, kFenceTimeout,
                                            imageAvailableSemaphore_, VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized_) {
        consecutiveSwapchainRecreations_++;
        std::string logMsg = "Swapchain recreation triggered (attempt " + std::to_string(consecutiveSwapchainRecreations_) + ")";
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            logMsg += " - OUT_OF_DATE";
        } else if (result == VK_SUBOPTIMAL_KHR) {
            logMsg += " - SUBOPTIMAL";
        } else if (framebufferResized_) {
            logMsg += " - RESIZE_EVENT";
        }
        sdl3cpp::logging::Logger::GetInstance().Info(logMsg);
        
        // Detect infinite swapchain recreation loop
        constexpr int kMaxConsecutiveRecreations = 10;
        if (consecutiveSwapchainRecreations_ > kMaxConsecutiveRecreations) {
            throw std::runtime_error(
                "Swapchain recreation loop detected: " + std::to_string(consecutiveSwapchainRecreations_) +
                " consecutive recreations. This may indicate a driver issue or window manager problem.\n"
                "Try running with a different window manager or updating your GPU drivers.");
        }
        
        RecreateSwapChain();
        return;
    } else if (result == VK_TIMEOUT) {
        std::cerr << "\nERROR: Image acquisition timeout: GPU appears to be hung\n";
        PrintGpuDiagnostics("Image acquisition timeout after 5 seconds");
        throw std::runtime_error("Image acquisition timeout: GPU appears to be hung");
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to acquire swap chain image");
    }
    sdl3cpp::logging::Logger::GetInstance().TraceVariable("imageIndex", imageIndex);

    float aspect = static_cast<float>(swapChainExtent_.width) / static_cast<float>(swapChainExtent_.height);
    auto viewProj = scriptEngine_.GetViewProjectionMatrix(aspect);

    vkResetCommandBuffer(commandBuffers_[imageIndex], 0);
    RecordCommandBuffer(commandBuffers_[imageIndex], imageIndex, time, viewProj);

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore_};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore_};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers_[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFence_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain_;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue_, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized_) {
        consecutiveSwapchainRecreations_++;
        std::cout << "Swapchain recreation after present (attempt " << consecutiveSwapchainRecreations_ << ")\n";
        
        constexpr int kMaxConsecutiveRecreations = 10;
        if (consecutiveSwapchainRecreations_ > kMaxConsecutiveRecreations) {
            throw std::runtime_error(
                "Swapchain recreation loop detected after present: " + std::to_string(consecutiveSwapchainRecreations_) +
                " consecutive recreations.");
        }
        
        RecreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image");
    } else {
        // Successfully presented a frame - reset counter
        if (consecutiveSwapchainRecreations_ > 0) {
            std::cout << "Frame presented successfully after " << consecutiveSwapchainRecreations_ 
                      << " swapchain recreation(s)\n";
        }
        consecutiveSwapchainRecreations_ = 0;
        
        if (!firstFrameCompleted_) {
            firstFrameCompleted_ = true;
            std::cout << "First frame completed successfully\n";
        }
    }
}

} // namespace sdl3cpp::app
