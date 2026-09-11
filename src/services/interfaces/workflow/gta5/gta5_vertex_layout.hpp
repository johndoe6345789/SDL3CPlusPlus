#pragma once

#include "services/interfaces/workflow/gta5/gta5_resource.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <cstdint>

namespace sdl3cpp::services::impl {

/// Where a G9 vertex keeps what the map shader needs.
///
/// The vertex buffer holds the count (+0x08), stride (+0x0C), data
/// (+0x18) and declaration (+0x38). A G9 declaration is 52 slots in fixed
/// semantic order -- 0 position, 4 normal, 24 and 25 colour 0 and 1, 28
/// texcoord 0 -- each with a u32 offset (+0), a u8 stride (+208) and a
/// u8 DXGI-style format (+260); format 0 is an empty slot.
struct Gta5VertexLayout {
    std::uint32_t count{0};
    std::uint32_t stride{0};
    std::int64_t data{-1};
    std::uint32_t position{0};
    std::uint32_t normal{0};
    std::uint32_t uv{0};
    std::uint32_t colour1{0};
    std::uint8_t normalFormat{0};
    std::uint8_t uvFormat{0};
    std::uint8_t colour1Format{0};
};

bool ReadGta5VertexLayout(const Gta5Resource& res, std::int64_t buffer,
                          Gta5VertexLayout& out);

/// One vertex in engine space: position and normal (x, z, -y), uv as
/// stored. Colour 1 -- terrain's layer weights -- rides in the unused
/// lightmap uv, two bytes to a float as r + 256 g and b + 256 a: whole
/// numbers that small are exact, and the vertex shader unpacks them.
BspRenderVertex ReadGta5Vertex(const Gta5Resource& res,
                               const Gta5VertexLayout& layout,
                               std::uint32_t index);

}  // namespace sdl3cpp::services::impl
