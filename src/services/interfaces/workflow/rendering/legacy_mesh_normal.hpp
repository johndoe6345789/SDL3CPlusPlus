#pragma once

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// Derives a legacy mesh's flat normal from the thinnest axis of its
/// `bb_min`/`bb_max` bounding box, leaving `normal` untouched when the node
/// has no bounding box.
void ApplyLegacyMeshNormal(const nlohmann::json& node, float normal[3]);

}  // namespace sdl3cpp::services::impl
