#pragma once

#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_mesh.hpp"

namespace sdl3cpp::services::impl {

/// One tree/shrub billboard: two crossed vertical quads, `width` x
/// `height`, centred at (x, z), their base at `groundY + offsetY`
/// (`offsetY` is FS2024's own relativeOffsetY, a fraction of `height`,
/// sinking the billboard so the atlas frame's empty margin does not
/// float above the real canopy). `layer` is the array layer to sample
/// (a variation's own textureIndex); the UV rect picks one frame of
/// the atlas's `frames` x `frames` grid, `frameCol` across and always
/// row 0 -- any real row is a usable view, so only column varies the
/// look between instances (see fs2024_vegetation_build.cpp). Not
/// billboarded to the camera: two fixed quads read reasonably from
/// most angles without a dedicated shader.
void AppendFs2024VegetationQuad(Fs2024TerrainChunkMesh& mesh, float x,
                                float z, float groundY, float width,
                                float height, float offsetY, int layer,
                                int frames, int frameCol);

}  // namespace sdl3cpp::services::impl
