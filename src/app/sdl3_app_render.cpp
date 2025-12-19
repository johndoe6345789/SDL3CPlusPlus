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
        pushConstants.model = cubeScript_.ComputeModelMatrix(object.computeModelMatrixRef, time);
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
    guiHasCommands_ = cubeScript_.HasGuiCommands();
    if (!guiHasCommands_) {
        guiRenderer_.reset();
        return;
    }
    if (!guiRenderer_) {
        guiRenderer_ =
            std::make_unique<gui::GuiRenderer>(device_, physicalDevice_, swapChainImageFormat_,
                                                cubeScript_.GetScriptDirectory());
    }
    guiRenderer_->Resize(swapChainExtent_.width, swapChainExtent_.height, swapChainImageFormat_);
}

void Sdl3App::DrawFrame(float time) {
    TRACE_FUNCTION();
    vkWaitForFences(device_, 1, &inFlightFence_, VK_TRUE, std::numeric_limits<uint64_t>::max());
    vkResetFences(device_, 1, &inFlightFence_);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device_, swapChain_, std::numeric_limits<uint64_t>::max(),
                                            imageAvailableSemaphore_, VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized_) {
        RecreateSwapChain();
        return;
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to acquire swap chain image");
    }
    TRACE_VAR(imageIndex);

    float aspect = static_cast<float>(swapChainExtent_.width) / static_cast<float>(swapChainExtent_.height);
    auto viewProj = cubeScript_.GetViewProjectionMatrix(aspect);

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
        RecreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image");
    }
}

} // namespace sdl3cpp::app
