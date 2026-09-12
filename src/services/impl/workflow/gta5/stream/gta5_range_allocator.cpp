#include "services/interfaces/workflow/gta5/stream/gta5_range_allocator.hpp"

#include <iterator>

namespace sdl3cpp::services::impl {

Gta5RangeAllocator::Gta5RangeAllocator(std::uint32_t capacity) {
    if (capacity > 0) free_[0] = capacity;
}

bool Gta5RangeAllocator::Allocate(std::uint32_t size, std::uint32_t& offset) {
    if (size == 0) return false;
    for (auto it = free_.begin(); it != free_.end(); ++it) {
        if (it->second < size) continue;
        offset = it->first;
        const std::uint32_t rest = it->second - size;
        free_.erase(it);
        if (rest > 0) free_[offset + size] = rest;
        return true;
    }
    return false;
}

void Gta5RangeAllocator::Free(std::uint32_t offset, std::uint32_t size) {
    if (size == 0) return;
    auto next = free_.lower_bound(offset);
    if (next != free_.begin()) {
        const auto prev = std::prev(next);
        if (prev->first + prev->second == offset) {
            offset = prev->first;
            size += prev->second;
            free_.erase(prev);
        }
    }
    if (next != free_.end() && offset + size == next->first) {
        size += next->second;
        free_.erase(next);
    }
    free_[offset] = size;
}

}  // namespace sdl3cpp::services::impl
