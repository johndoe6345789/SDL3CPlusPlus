#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"

#include <SDL3/SDL_gpu.h>

class btDiscreteDynamicsWorld;

namespace sdl3cpp::services::impl {

/// How the main thread's share of streaming went.
struct Fs2024FinishStats {
    int finished = 0;
    double worstMs = 0.0;  ///< the slowest single finish
};

/// Takes tiles the loader threads have built and finishes them -- one at
/// a time, until `budgetMs` of this frame is spent (at least one is
/// always taken, so streaming never stalls on a slow frame). A tile no
/// longer wanted is thrown away unfinished; one that failed is marked
/// missing.
void FinishFs2024ReadyTiles(Fs2024TileStreamState& state,
                            SDL_GPUDevice* device,
                            btDiscreteDynamicsWorld* physics,
                            double budgetMs, ILogger* logger,
                            Fs2024FinishStats& stats);

}  // namespace sdl3cpp::services::impl
