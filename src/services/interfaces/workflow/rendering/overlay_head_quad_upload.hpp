#pragma once

#include "services/interfaces/workflow/rendering/overlay_sw_end_resources.hpp"

namespace sdl3cpp::services::impl {

/// Uploads a 6-vertex quad for the face rect, in overlay-pixel NDC.
/// Lazily creates `res.headVertices` on first use. Returns false if buffer
/// creation or the transfer-buffer upload fails.
bool UploadHeadQuad(SDL_GPUCommandBuffer* cmd, OverlaySwEndResources& res,
                    float faceRectX, float faceRectY, float faceRectW,
                    float faceRectH, int overlayWidth, int overlayHeight);

}  // namespace sdl3cpp::services::impl
