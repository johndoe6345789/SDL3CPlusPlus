#pragma once

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// A GPU index buffer plus the byte size that was uploaded to it, so the
/// caller can report it without re-deriving it from the source data.
struct IndexBufferUploadResult {
    SDL_GPUBuffer* buffer = nullptr;
    uint32_t size_bytes = 0;
};

/**
 * @brief Converts a JSON array of index numbers into uint16 index data.
 *
 * @throws std::runtime_error if any entry is not a number, or the array
 *         is empty.
 */
std::vector<uint16_t> ExtractIndexData(const nlohmann::json& indices_json);

/**
 * @brief Creates a GPU index buffer and uploads `index_data` into it via
 *        a transfer buffer.
 *
 * @throws std::runtime_error if buffer/transfer-buffer creation fails.
 */
IndexBufferUploadResult CreateAndUploadIndexBuffer(
    SDL_GPUDevice* device, const std::vector<uint16_t>& index_data);

}  // namespace sdl3cpp::services::impl
