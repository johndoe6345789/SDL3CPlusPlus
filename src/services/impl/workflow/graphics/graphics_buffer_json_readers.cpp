#include "services/interfaces/workflow/graphics/graphics_buffer_json_readers.hpp"

#include <stdexcept>

using json = nlohmann::json;

namespace sdl3cpp::services::impl {

std::vector<uint8_t> ReadVertexBytesFromContext(const WorkflowContext& context,
                                                const std::string& key) {
    const auto* vertexJson = context.TryGet<json>(key);
    if (!vertexJson || !vertexJson->is_array() || vertexJson->empty()) {
        throw std::runtime_error(
            "graphics.buffer.upload: '" + key +
            "' not found or not a non-empty array in context");
    }

    std::vector<uint8_t> vertexBytes;
    vertexBytes.reserve(vertexJson->size());
    for (const auto& v : *vertexJson) {
        if (!v.is_number()) {
            throw std::runtime_error(
                "graphics.buffer.upload: vertex data must be array of "
                "numbers");
        }
        vertexBytes.push_back(static_cast<uint8_t>(v.get<int>()));
    }
    return vertexBytes;
}

std::vector<uint16_t> ReadIndexValuesFromContext(const WorkflowContext& context,
                                                 const std::string& key) {
    const auto* indexJson = context.TryGet<json>(key);
    if (!indexJson || !indexJson->is_array() || indexJson->empty()) {
        throw std::runtime_error(
            "graphics.buffer.upload: '" + key +
            "' not found or not a non-empty array in context");
    }

    std::vector<uint16_t> indexValues;
    indexValues.reserve(indexJson->size());
    for (const auto& idx : *indexJson) {
        if (!idx.is_number()) {
            throw std::runtime_error(
                "graphics.buffer.upload: index data must be array of "
                "numbers");
        }
        indexValues.push_back(static_cast<uint16_t>(idx.get<int>()));
    }
    return indexValues;
}

}  // namespace sdl3cpp::services::impl
