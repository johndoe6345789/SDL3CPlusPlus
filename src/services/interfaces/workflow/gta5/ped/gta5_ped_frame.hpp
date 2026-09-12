#pragma once

#include "services/interfaces/workflow/gta5/ped/gta5_ped.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

/// One vertex's skinning: four bones by their index at `indices` into the
/// geometry's bone ids (`ids`, `idCount` of them), weights at `weights`.
void ReadGta5PedBlend(const Gta5Resource& res, std::int64_t weights,
                      std::int64_t indices, std::int64_t ids,
                      std::uint16_t idCount, Gta5PedPart& part);

/// A part's bind pose into every slot of its ring, and its submesh. The
/// texture is the ped's own, kept in `owned`: the map's texture cache may
/// drop what it holds.
bool UploadGta5PedPart(Gta5StreamState& state, SDL_GPUDevice* device,
                       const Gta5Resource& ytd, Gta5PedPart& part,
                       Gta5SubMesh& sub, std::vector<SDL_GPUTexture*>& owned);

/// Skin every part by `skin` into its ring's slot `ring`, staged for this
/// frame, and point the submeshes at those slots.
void StageGta5PedFrame(Gta5StreamState& state, SDL_GPUDevice* device,
                       Gta5Ped& ped, const std::vector<glm::mat4>& skin,
                       int ring, std::vector<BspRenderVertex>& scratch);

}  // namespace sdl3cpp::services::impl
