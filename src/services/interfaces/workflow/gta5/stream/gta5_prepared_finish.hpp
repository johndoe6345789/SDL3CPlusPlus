#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_load_budget.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <SDL3/SDL_gpu.h>

#include <memory>

namespace sdl3cpp::services::impl {

/// Take what the load pool has finished and upload it, on this thread,
/// until the frame's time or bytes run out.
///
/// Textures go first, one at a time, since they are what prepared
/// geometry waits on; then every prepared archetype whose textures have
/// all landed is uploaded and its worker-built collision moved in. The
/// budget is checked between single uploads: it was checked between
/// batches of eight, and one batch could spend 14 ms.
void FinishGta5PreparedGeometry(Gta5StreamState& state, SDL_GPUDevice* device,
                                const Gta5LoadBudget& limits,
                                const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
