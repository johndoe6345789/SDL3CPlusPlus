#pragma once

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>

#include <vector>

namespace sdl3cpp::services::impl {

/// Converts a JSON array of numbers into a flat float vector. Throws
/// std::runtime_error if any element isn't a number.
std::vector<float> ParseVertexFloats(const nlohmann::json& verticesJson);

/**
 * @brief Creates a GPU vertex buffer sized to `data` and uploads it via one
 * transfer buffer and copy pass.
 *
 * Throws std::runtime_error (prefixed "graphics.buffer.create_vertex: ...")
 * if buffer or transfer-buffer creation fails.
 */
SDL_GPUBuffer* UploadVertexBuffer(SDL_GPUDevice* device,
                                  const std::vector<float>& data);

}  // namespace sdl3cpp::services::impl
