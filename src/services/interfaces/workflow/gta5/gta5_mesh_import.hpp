#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_placement.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Load one exported drawable off disk into `outGeometry`.
///
/// `archetype` is used only for log messages. Returns false and leaves the
/// geometry unusable on any failure, including a mesh too large to index.
bool ImportGta5Mesh(const std::string& path, const std::string& archetype,
                    Gta5Geometry& outGeometry,
                    const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
