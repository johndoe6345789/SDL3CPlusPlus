#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/scene_types.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

struct Gta5SpawnOptions {
    std::string shaderKey{"gpu_pipeline_textured"};
    std::string objectTypePrefix;
};

/// Spawn up to `budget` not-yet-spawned placements of one tile, appending
/// them to `objects`. Returns how many placements were consumed.
///
/// A placement is consumed whether or not it produced an object: a missing
/// or oversized model must not be retried every frame.
int SpawnGta5TilePlacements(Gta5StreamState& state,
                            const Gta5TileCoord& tile,
                            Gta5ResidentTile& resident,
                            const Gta5SpawnOptions& options, int budget,
                            std::vector<SceneObject>& objects,
                            const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
