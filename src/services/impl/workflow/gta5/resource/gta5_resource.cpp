#include "services/interfaces/workflow/gta5/resource/gta5_resource.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

template <typename T>
T ReadAt(const std::vector<std::uint8_t>& data, std::int64_t at) {
    T value{};
    if (at < 0 || static_cast<std::uint64_t>(at) + sizeof(T) > data.size()) {
        return value;
    }
    std::memcpy(&value, data.data() + at, sizeof(T));
    return value;
}

}  // namespace

std::int64_t Gta5Resource::Resolve(std::uint64_t pointer) const {
    const auto low = static_cast<std::uint32_t>(pointer);
    const std::int64_t offset = low & 0x0FFFFFFFu;
    std::int64_t flat = -1;
    if ((low >> 28) == 0x5u) flat = offset;
    if ((low >> 28) == 0x6u) flat = systemSize + offset;
    if (flat < 0 || static_cast<std::uint64_t>(flat) >= data.size()) return -1;
    return flat;
}

std::vector<std::int64_t> Gta5Resource::PointerList(std::int64_t at) const {
    std::vector<std::int64_t> out;
    const std::int64_t array = Follow(at);
    if (array < 0) return out;
    const std::uint16_t count = U16(at + 8);
    out.reserve(count);
    for (std::uint16_t i = 0; i < count; ++i) {
        out.push_back(Follow(array + 8 * static_cast<std::int64_t>(i)));
    }
    return out;
}

std::uint8_t Gta5Resource::U8(std::int64_t at) const {
    return ReadAt<std::uint8_t>(data, at);
}
std::uint16_t Gta5Resource::U16(std::int64_t at) const {
    return ReadAt<std::uint16_t>(data, at);
}
std::uint32_t Gta5Resource::U32(std::int64_t at) const {
    return ReadAt<std::uint32_t>(data, at);
}
std::uint64_t Gta5Resource::U64(std::int64_t at) const {
    return ReadAt<std::uint64_t>(data, at);
}
float Gta5Resource::F32(std::int64_t at) const {
    return ReadAt<float>(data, at);
}

}  // namespace sdl3cpp::services::impl
