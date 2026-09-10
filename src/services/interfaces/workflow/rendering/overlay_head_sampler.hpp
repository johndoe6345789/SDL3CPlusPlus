#pragma once

#include "services/interfaces/workflow/rendering/overlay_sw_end_resources.hpp"

namespace sdl3cpp::services::impl {

/// Lazily creates `res.headSampler` (linear, clamp-to-edge) on first use.
/// Returns false only if sampler creation fails.
bool EnsureHeadSampler(OverlaySwEndResources& res);

}  // namespace sdl3cpp::services::impl
