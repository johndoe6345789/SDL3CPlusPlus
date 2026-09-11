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
/// and 29 texcoord 0 and 1 -- each with a u32 offset (+0), a u8 stride
/// (+208) and a u8 DXGI-style format (+260); format 0 is an empty slot.
struct Gta5VertexLayout {
    std::uint32_t count{0};
    std::uint32_t stride{0};
    std::int64_t data{-1};
    std::uint32_t position{0}, normal{0}, uv{0}, uv1{0};
    std::uint32_t colour0{0}, colour1{0};
    std::uint8_t normalFormat{0}, uvFormat{0}, uv1Format{0};
    std::uint8_t colour0Format{0}, colour1Format{0};
};

bool ReadGta5VertexLayout(const Gta5Resource& res, std::int64_t buffer,
                          Gta5VertexLayout& out);

/// One vertex in engine space: position and normal (x, z, -y), uv as
/// stored. Terrain's data rides in the unused lightmap uv as whole
/// numbers, which a float holds exactly below 2^24: lm_u is colour 1's
/// blue + 256 green -- the layer weights -- + 65536 colour 0's alpha --
/// how far a lookup mask gives way to them; lm_v is texcoord 1, where the
/// mask is read, as 12 bits each of u and v over [0, 1].
BspRenderVertex ReadGta5Vertex(const Gta5Resource& res,
                               const Gta5VertexLayout& layout,
                               std::uint32_t index);

}  // namespace sdl3cpp::services::impl
