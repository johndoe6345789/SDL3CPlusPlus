#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <set>
#include <string>

// Forward-declared to avoid pulling <zip.h> into every includer.
typedef struct zip zip_t;

namespace sdl3cpp::services::impl {

/// Outcome of loading every texture bsp.build_geometry referenced.
struct BspTextureBatchResult {
    int loadedCount    = 0;
    int missingCount   = 0;
    int viaShaderCount = 0;
};

/**
 * @brief Loads and publishes every texture index in `usedTextures`.
 *
 * For each index, publishes `bsp_tex_{idx}_gpu`/`_sampler` — either the
 * decoded pk3 image (see LoadBspTextureFromPk3) or a white fallback when the
 * pk3 has nothing under that name or its shader-script mapping.
 */
BspTextureBatchResult LoadBspTextureBatch(zip_t* archive, SDL_GPUDevice* device,
                                          const std::set<int>& usedTextures,
                                          const BspTexture* bspTextures,
                                          int numTextures,
                                          WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
