#pragma once

#include "services/interfaces/workflow/gta5/render/gta5_draw_instances.hpp"

namespace sdl3cpp::services::impl {

/// Bind what every draw of the batch shares -- its instance matrices and
/// the camera uniforms -- for the pipeline just bound. Called again
/// after each pipeline switch.
void BindGta5BatchShared(const Gta5DrawContext& draw,
                         const Gta5InstanceBatch& batch);

/// Bind one geometry arena block's vertex and index buffers.
void BindGta5ArenaBlock(const Gta5StreamState& state,
                        const Gta5DrawContext& draw, int block);

/// The textures the last draw bound, so the next can skip them.
struct Gta5BoundTextures {
    SDL_GPUTexture* diffuse{nullptr};
    SDL_GPUTexture* bump{nullptr};
    SDL_GPUTexture* specular{nullptr};
};

/// Bind a submesh's textures: its diffuse, normal and specular maps, or
/// for terrain all four layers and the mask, any missing one standing in
/// with the diffuse. Returns the binds made, or -1 when there is nothing
/// to sample and the draw should be skipped.
int BindGta5SubMeshTextures(const Gta5DrawContext& draw,
                            const Gta5SubMesh& sub,
                            Gta5BoundTextures& bound);

/// Puts the submesh's tint and alpha threshold (the spotlight colour
/// slot) and whether it has normal and specular maps (material x, y)
/// into `fu`. True when that changed anything.
bool SetGta5SurfaceUniforms(rendering::FragmentUniformData& fu,
                            const Gta5SubMesh& sub);

}  // namespace sdl3cpp::services::impl
