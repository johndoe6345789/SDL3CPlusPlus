#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>

namespace sdl3cpp::vulkan::utils {

VkExtent2D ChooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities, SDL_Window* window);

void CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

} // namespace sdl3cpp::vulkan::utils