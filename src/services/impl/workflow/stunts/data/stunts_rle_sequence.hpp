#pragma once

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Expands `escape1 ... escape1 [count]` runs into `count` copies of
/// the bytes between the markers. Bytes outside any such run pass
/// through unchanged, including any other escape codes' bytes -- they
/// are handled by the run-length pass that follows this one.
std::vector<std::uint8_t> ExpandStuntsSequences(
    const std::vector<std::uint8_t>& body, std::uint8_t marker);

}  // namespace sdl3cpp::services::impl
