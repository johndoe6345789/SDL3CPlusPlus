#pragma once

/// Internal helpers shared by the overlay_sw_end_resources_*.cpp files that
/// together implement overlay_sw_end_resources.hpp. Not part of the public
/// workflow-step API.

#include "services/interfaces/workflow/rendering/overlay_sw_end_resources.hpp"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl::overlay_sw_end_detail {

/// Reads an entire file into memory; returns an empty vector on any error.
std::vector<uint8_t> LoadBinary(const std::string& path);

/// Builds the alpha-blended textured-quad pipeline used for both the
/// overlay surface blit and the head-portrait blit.
SDL_GPUGraphicsPipeline* CreatePipeline(SDL_GPUDevice* device,
                                        SDL_Window* window,
                                        SDL_GPUShader* vertex,
                                        SDL_GPUShader* fragment);

/// Loads, compiles and links the vertex/fragment shader pair into a
/// pipeline; returns nullptr if the device's backend is unsupported, a
/// shader binary is missing, or shader/pipeline creation fails.
SDL_GPUGraphicsPipeline* CreateShaderStage(SDL_GPUDevice* device,
                                           SDL_Window* window,
                                           const std::string& vertPath,
                                           const std::string& fragPath);

/// Creates the texture, transfer buffer, vertex buffer and sampler on
/// `res`; returns false if any allocation failed.
bool CreateGpuBuffers(SDL_GPUDevice* device, int surfaceWidth,
                      int surfaceHeight, OverlaySwEndResources& res);

}  // namespace sdl3cpp::services::impl::overlay_sw_end_detail
