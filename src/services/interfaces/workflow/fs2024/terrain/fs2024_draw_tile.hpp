#pragma once

#include "services/interfaces/workflow/fs2024/terrain/fs2024_frustum.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_ground_uniforms.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>

#include <string>

namespace sdl3cpp::services::impl {

/// Draws one tile's ground through the ground pipeline (bound by the
/// caller): pushes the tile's origin offset, binds its class map and
/// the world's material array, draws every chunk the frustum can see.
void DrawFs2024TileGround(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                          const Fs2024LoadedTile& tile,
                          const Fs2024World& world,
                          Fs2024TerrainVertexUniforms vertex,
                          const Fs2024Frustum& frustum);

/// The texture a step published under `key` (`<key>_gpu` and
/// `<key>_sampler`), or an empty binding.
SDL_GPUTextureSamplerBinding Fs2024ContextTexture(
    const WorkflowContext& context, const std::string& key);

/// Draws one tile's walls and roofs through the terrain pipeline (bound
/// by the caller), each with its own shared texture.
void DrawFs2024TileBuildings(SDL_GPURenderPass* pass,
                             SDL_GPUCommandBuffer* cmd,
                             const Fs2024LoadedTile& tile,
                             Fs2024TerrainVertexUniforms vertex,
                             const Fs2024TerrainFragmentUniforms& fragment,
                             const Fs2024Frustum& frustum,
                             SDL_GPUTextureSamplerBinding wall,
                             SDL_GPUTextureSamplerBinding roof);

}  // namespace sdl3cpp::services::impl
