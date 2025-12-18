#ifndef SDL3CPP_APP_VULKAN_API_HPP
#define SDL3CPP_APP_VULKAN_API_HPP

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

namespace sdl3cpp::app::vulkan {

VkExtent2D ChooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities, SDL_Window* window);

void CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

} // namespace sdl3cpp::app::vulkan

#endif // SDL3CPP_APP_VULKAN_API_HPP
