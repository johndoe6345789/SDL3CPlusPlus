#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::tools::fs2024 {

/// One `id`+payload chunk of a RIFF-style container (used by both the
/// outer `RIFF....GLTF` blob a BGL ModelData record holds, and the
/// `GLBD` chunk nested inside it, which holds one `GLB\0` chunk per
/// LOD). `data`/`size` point into the buffer `WalkRiffChunks` was
/// given -- they do not own it.
struct RiffChunk {
    std::string id;  ///< 4 bytes, e.g. "GXML", "GLBD", "GLB\0"
    const std::uint8_t* data = nullptr;
    std::size_t size = 0;
};

/// Walks the chunks starting at `data[0]` (already past any outer
/// `RIFF <size> <form>` header the caller has stripped) up to
/// `data[size]`. Each chunk is `id(4) + length(u32) + payload`,
/// payload padded to an even length, exactly as RIFF/WAV chunks are.
/// A chunk whose declared length would run past `size` is dropped
/// rather than thrown, so a short/probe-only buffer can still be
/// walked for whichever chunks are fully present.
std::vector<RiffChunk> WalkRiffChunks(const std::uint8_t* data,
                                     std::size_t size);

}  // namespace sdl3cpp::tools::fs2024
