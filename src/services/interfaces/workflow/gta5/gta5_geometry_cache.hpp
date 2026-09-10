#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/// Fetch an archetype's mesh, loading it on first use.
///
/// Returns nullptr when the archetype has no exported model or failed to
/// load. Failures are cached as unusable, so a broken asset costs one load
/// attempt for the session rather than one per instance per frame.
const Gta5Geometry* GetOrLoadGta5Geometry(
    Gta5StreamState& state, const Gta5Placement& placement,
    const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
