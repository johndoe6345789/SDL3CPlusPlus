#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <map>
#include <memory>

namespace sdl3cpp::services::impl {

/// Context key shared by bsp.tessellate_patches, bsp.build_polygons and
/// bsp.flatten_geometry for the per-texture geometry being assembled.
inline constexpr const char* kBspTextureGroupsKey = "bsp_texture_groups";

/// Fetches the shared group map, creating and publishing it if absent.
std::shared_ptr<std::map<int, TextureGroup>> GetOrCreateBspTextureGroups(
    WorkflowContext& context);

/// Fetches the shared group map, or null if no step has created it yet.
std::shared_ptr<std::map<int, TextureGroup>> GetBspTextureGroups(
    WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
