#include "app/sdl3_app.hpp"
#include "app/trace.hpp"

#include <limits>
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
    std::cerr << "\n========================================\n";
    std::cerr << "GPU DIAGNOSTIC REPORT\n";
    std::cerr << "========================================\n";
    std::cerr << "Error Context: " << errorContext << "\n\n";
    
    // Device properties
    if (physicalDevice_ != VK_NULL_HANDLE) {
        VkPhysicalDeviceProperties deviceProps{};
        vkGetPhysicalDeviceProperties(physicalDevice_, &deviceProps);
        
        std::cerr << "=== GPU Information ===\n";
        std::cerr << "Device Name: " << deviceProps.deviceName << "\n";
        std::cerr << "Driver Version: " << VK_API_VERSION_MAJOR(deviceProps.driverVersion) << "."
                  << VK_API_VERSION_MINOR(deviceProps.driverVersion) << "."
                  << VK_API_VERSION_PATCH(deviceProps.driverVersion) << "\n";
        std::cerr << "API Version: " << VK_API_VERSION_MAJOR(deviceProps.apiVersion) << "."
                  << VK_API_VERSION_MINOR(deviceProps.apiVersion) << "."
                  << VK_API_VERSION_PATCH(deviceProps.apiVersion) << "\n";
        std::cerr << "Vendor ID: 0x" << std::hex << deviceProps.vendorID << std::dec << "\n";
        std::cerr << "Device ID: 0x" << std::hex << deviceProps.deviceID << std::dec << "\n";
        
        VkPhysicalDeviceMemoryProperties memProps{};
        vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProps);
        
        std::cerr << "\n=== Memory Information ===\n";
        uint64_t totalVRAM = 0;
        for (uint32_t i = 0; i < memProps.memoryHeapCount; i++) {
            if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                totalVRAM += memProps.memoryHeaps[i].size;
            }
        }
        std::cerr << "Total VRAM: " << (totalVRAM / 1024 / 1024) << " MB\n";
        
        // Memory heaps breakdown
        std::cerr << "Memory Heaps (" << memProps.memoryHeapCount << "):\n";
        for (uint32_t i = 0; i < memProps.memoryHeapCount; i++) {
            std::cerr << "  Heap " << i << ": " << (memProps.memoryHeaps[i].size / 1024 / 1024) << " MB";
            if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                std::cerr << " (Device Local)";
            }
            std::cerr << "\n";
        }
    }
    
    // Swapchain state
    std::cerr << "\n=== Swapchain State ===\n";
    std::cerr << "Extent: " << swapChainExtent_.width << "x" << swapChainExtent_.height << "\n";
    std::cerr << "Image Count: " << swapChainImages_.size() << "\n";
    std::cerr << "Format: " << swapChainImageFormat_ << "\n";
    std::cerr << "Consecutive Recreations: " << consecutiveSwapchainRecreations_ << "\n";
    std::cerr << "Framebuffer Resized Flag: " << (framebufferResized_ ? "true" : "false") << "\n";
    std::cerr << "First Frame Completed: " << (firstFrameCompleted_ ? "true" : "false") << "\n";
    
    // Render objects
    std::cerr << "\n=== Scene State ===\n";
    std::cerr << "Render Objects: " << renderObjects_.size() << "\n";
    std::cerr << "Vertices: " << vertices_.size() << "\n";
    std::cerr << "Indices: " << indices_.size() << "\n";
    std::cerr << "Pipelines: " << graphicsPipelines_.size() << "\n";
    std::cerr << "GUI Renderer Active: " << (guiRenderer_ ? "true" : "false") << "\n";
    std::cerr << "GUI Has Commands: " << (guiHasCommands_ ? "true" : "false") << "\n";
    
    // Check device features that might be related
    if (physicalDevice_ != VK_NULL_HANDLE) {
        VkPhysicalDeviceFeatures deviceFeatures{};
        vkGetPhysicalDeviceFeatures(physicalDevice_, &deviceFeatures);
        
        std::cerr << "\n=== Relevant Device Features ===\n";
        std::cerr << "Geometry Shader: " << (deviceFeatures.geometryShader ? "supported" : "not supported") << "\n";
        std::cerr << "Tessellation Shader: " << (deviceFeatures.tessellationShader ? "supported" : "not supported") << "\n";
        std::cerr << "Multi Draw Indirect: " << (deviceFeatures.multiDrawIndirect ? "supported" : "not supported") << "\n";
    }
    
    // Queue properties
    if (physicalDevice_ != VK_NULL_HANDLE) {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, queueFamilies.data());
        
        std::cerr << "\n=== Queue Families ===\n";
        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            std::cerr << "Family " << i << ": " << queueFamilies[i].queueCount << " queues, flags: 0x"
                      << std::hex << queueFamilies[i].queueFlags << std::dec << "\n";
        }
    }
    
    std::cerr << "\n=== Possible Causes ===\n";
    std::cerr << "1. GPU driver crash or hang - Check dmesg for GPU reset messages\n";
    std::cerr << "2. Infinite loop in shader code - Review vertex/fragment shaders\n";
    std::cerr << "3. Command buffer submission issue - Check synchronization\n";
    std::cerr << "4. GPU overheating or hardware issue - Monitor GPU temperature\n";
    std::cerr << "5. Driver bug - Try updating GPU drivers to latest version\n";
    std::cerr << "6. Resource exhaustion - Check system memory and VRAM usage\n";
    
    std::cerr << "\n=== Recommended Actions ===\n";
    std::cerr << "1. Check system logs: dmesg | grep -i 'gpu\\|radeon\\|amdgpu'\n";
    std::cerr << "2. Update GPU drivers: sudo dnf update mesa-vulkan-drivers\n";
    std::cerr << "3. Verify GPU health: radeontop or similar monitoring tool\n";
    std::cerr << "4. Check for driver messages: journalctl -k | grep -i amdgpu\n";
    std::cerr << "5. Try with different Vulkan settings or validation layers\n";
    std::cerr << "========================================\n\n";
}

void Sdl3App::CreateCommandBuffers() {
    TRACE_FUNCTION();
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
    TRACE_FUNCTION();
    TRACE_VAR(imageIndex);
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
    vkCmdEndRenderPass(commandBuffer);
    if (guiRenderer_) {
        guiRenderer_->BlitToSwapchain(commandBuffer, swapChainImages_[imageIndex]);
    }
    vkEndCommandBuffer(commandBuffer);
}

void Sdl3App::ProcessGuiEvent(const SDL_Event& event) {
    TRACE_FUNCTION();
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
    TRACE_FUNCTION();
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
    TRACE_FUNCTION();
    
    // Use reasonable timeout instead of infinite wait (5 seconds)
    constexpr uint64_t kFenceTimeout = 5000000000ULL; // 5 seconds in nanoseconds
    VkResult fenceResult = vkWaitForFences(device_, 1, &inFlightFence_, VK_TRUE, kFenceTimeout);
    if (fenceResult == VK_TIMEOUT) {
        std::cerr << "\nERROR: Fence wait timeout: GPU appears to be hung\n";
        PrintGpuDiagnostics("Fence wait timeout after 5 seconds");
        throw std::runtime_error("Fence wait timeout: GPU appears to be hung");
    } else if (fenceResult != VK_SUCCESS) {
        std::cerr << "\nERROR: Fence wait failed with code: " << fenceResult << "\n";
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
        std::cout << "Swapchain recreation triggered (attempt " << consecutiveSwapchainRecreations_ << ")";
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            std::cout << " - OUT_OF_DATE";
        } else if (result == VK_SUBOPTIMAL_KHR) {
            std::cout << " - SUBOPTIMAL";
        } else if (framebufferResized_) {
            std::cout << " - RESIZE_EVENT";
        }
        std::cout << "\n";
        
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
    TRACE_VAR(imageIndex);

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
