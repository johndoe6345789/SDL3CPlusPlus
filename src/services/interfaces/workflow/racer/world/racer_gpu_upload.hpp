#pragma once

#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

namespace sdl3cpp::services::impl {

/// Copies `bytes` into a new vertex buffer. Null on failure.
SDL_GPUBuffer* UploadRacerVertexBuffer(SDL_GPUDevice* device,
                                       const std::vector<RacerGpuVertex>& v);

/// Clears the alpha of a texture whose transparent texels are only a
/// stray few (under 1 in 16): opaque art the cut-out test would
/// otherwise pierce. True when it did.
bool SolidifyRacerTexture(RacerTexture& texture);

/// The texture for a material: decoded, upscaled by the state's
/// `textureScale` (Scale2x passes), mipmapped, and cached by material.
RacerGpuTexture AcquireRacerTexture(SDL_GPUDevice* device,
                                    RacerWorldState& state,
                                    const RacerMaterialRef& material);

/// Uploads one RGBA image, upscaled by `scale` (a power of two, or 1),
/// with a full mip chain and an anisotropic repeating sampler.
RacerGpuTexture UploadRacerTexture(SDL_GPUDevice* device, RacerTexture t,
                                   int scale);

/// How a model's vertex colour bytes are meant.
enum class RacerVertexShading {
    Colour,   ///< baked lighting (tracks, scenery)
    Normal,   ///< signed normals, for models the game lights (pods)
};

/// Uploads every batch of a model, converted to engine space. When
/// `ground` is given, its walkable triangles are added to it as well.
RacerGpuModel UploadRacerModel(SDL_GPUDevice* device, RacerWorldState& state,
                               const RacerModel& model, RacerGround* ground,
                               RacerVertexShading shading =
                                   RacerVertexShading::Colour);

/// Releases every GPU object the state owns.
void ReleaseRacerWorld(SDL_GPUDevice* device, RacerWorldState& state);

}  // namespace sdl3cpp::services::impl
