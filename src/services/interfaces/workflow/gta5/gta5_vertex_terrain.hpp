#pragma once

#include "services/interfaces/workflow/gta5/gta5_vertex_layout.hpp"

#include <cstdint>

namespace sdl3cpp::services::impl {

/// A float2 or half2 texture coordinate at `at`. False, leaving u and v,
/// for any other format or an empty slot.
bool ReadGta5Uv(const Gta5Resource& res, std::int64_t at,
                std::uint8_t format, float& u, float& v);

/// Pack terrain's blend data into the vertex's lightmap uv; see
/// ReadGta5Vertex. Without colour 1 -- the _cm shaders -- the weights read
/// mid-grey and colour 0's alpha 0, so the lookup mask alone decides.
void PackGta5TerrainVertex(const Gta5Resource& res,
                           const Gta5VertexLayout& layout, std::int64_t at,
                           BspRenderVertex& v);

}  // namespace sdl3cpp::services::impl
