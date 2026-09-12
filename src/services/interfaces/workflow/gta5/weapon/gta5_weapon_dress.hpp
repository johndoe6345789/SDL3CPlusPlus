#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_geometry.hpp"
#include "services/interfaces/workflow/gta5/render/gta5_mesh_extract.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Hand a weapon's parts the textures from its own .ytd, which sits
/// beside the model in `dir` and which the map's index never scans.
void DressGta5Weapon(Gta5StreamState& state, SDL_GPUDevice* device,
                     const std::string& dir, const std::string& model,
                     const Gta5MeshData& mesh, Gta5Geometry& geometry,
                     const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
