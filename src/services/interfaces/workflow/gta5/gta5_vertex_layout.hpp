#pragma once

#include "services/interfaces/workflow/gta5/gta5_resource.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <cstdint>

namespace sdl3cpp::services::impl {

/// Where a G9 vertex keeps what the map shader needs.
///
/// The vertex buffer holds the count (+0x08), stride (+0x0C), data
/// (+0x18) and declaration (+0x38). A G9 declaration is 52 slots in fixed
/// semantic order -- 0 position, 4 normal, 28 texcoord 0 -- each with a
/// u32 offset (+0), a u8 stride (+208) and a u8 DXGI-style format (+260);
/// format 0 is an empty slot. Read from the resource rather than guessed
/// from the bytes, which is how the Python converter used to find them.
struct Gta5VertexLayout {
    std::uint32_t count{0};
    std::uint32_t stride{0};
    std::int64_t data{-1};
    std::uint32_t position{0};
    std::uint32_t normal{0};
    std::uint32_t uv{0};
    std::uint8_t normalFormat{0};
    std::uint8_t uvFormat{0};
};

bool ReadGta5VertexLayout(const Gta5Resource& res, std::int64_t buffer,
                          Gta5VertexLayout& out);

/// One vertex in engine space: position and normal (x, z, -y), and v
/// flipped, as assimp's FlipUVs did on the glTF path the shaders were
/// tuned against.
BspRenderVertex ReadGta5Vertex(const Gta5Resource& res,
                               const Gta5VertexLayout& layout,
                               std::uint32_t index);

}  // namespace sdl3cpp::services::impl
