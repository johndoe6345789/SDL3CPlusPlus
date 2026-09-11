#pragma once

#include <cstdint>
#include <cstring>

namespace sdl3cpp::services::impl {

/// IEEE 754 half to float. Some G9 vertex declarations store texcoords as
/// R16G16_FLOAT to halve the buffer.
inline float Gta5HalfToFloat(std::uint16_t half) {
    const std::uint32_t sign = (half & 0x8000u) << 16;
    const std::uint32_t exponent = (half >> 10) & 0x1Fu;
    const std::uint32_t mantissa = half & 0x3FFu;
    if (exponent == 0) {
        // Zero or subnormal: mantissa * 2^-24.
        const float value = static_cast<float>(mantissa) / 16777216.f;
        return (half & 0x8000u) ? -value : value;
    }
    std::uint32_t bits = sign | (mantissa << 13);
    bits |= (exponent == 0x1Fu) ? 0x7F800000u : ((exponent + 112u) << 23);
    float out;
    std::memcpy(&out, &bits, sizeof(out));
    return out;
}

}  // namespace sdl3cpp::services::impl
