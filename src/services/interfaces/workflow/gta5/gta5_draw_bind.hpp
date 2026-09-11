#pragma once

#include "services/interfaces/workflow/gta5/gta5_draw_instances.hpp"

namespace sdl3cpp::services::impl {

/// Bind what every draw of the batch shares -- its instance matrices and
/// the camera uniforms -- for the pipeline just bound. Called again
/// after each pipeline switch.
void BindGta5BatchShared(const Gta5StreamState& state,
                         const Gta5DrawContext& draw);

/// Bind one geometry arena block's vertex and index buffers.
void BindGta5ArenaBlock(const Gta5StreamState& state,
                        const Gta5DrawContext& draw, int block);

/// Bind a submesh's textures: its diffuse, or for terrain all four
/// layers, any missing one standing in with its diffuse. `bound` skips
/// rebinding the diffuse the last draw used. Returns the binds made, or
/// -1 when there is nothing to sample and the draw should be skipped.
int BindGta5SubMeshTextures(const Gta5DrawContext& draw,
                            const Gta5SubMesh& sub, SDL_GPUTexture*& bound);

}  // namespace sdl3cpp::services::impl
