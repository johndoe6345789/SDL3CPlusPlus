#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <SDL3/SDL_gpu.h>

#include <memory>

namespace sdl3cpp::services::impl {

/// Upload what the load pool has finished, on this thread, for up to
/// `budgetMs` of the frame.
///
/// Textures go first, since they are what prepared geometry waits on.
/// Then every prepared archetype whose textures have all landed is
/// uploaded and its worker-built collision moved in. One still waiting
/// on a texture another job is reading stays queued, rather than being
/// drawn untextured for good.
void FinishGta5PreparedGeometry(Gta5StreamState& state, SDL_GPUDevice* device,
                                float budgetMs,
                                const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
