#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

/// draw.map's per-mesh-name -> texture-name overrides, plus the fallback
/// texture used when no override matches (or, in BSP mode, when a texture
/// index has no loaded GPU texture).
struct DrawMapTextureConfig {
    std::vector<std::pair<std::string, std::string>> mappings;
    std::string defaultTexture;
    float roughness = 0.8f;
    float metallic  = 0.0f;
};

/// Reads the `default_texture`/`<mesh-pattern>` string parameters plus the
/// `roughness`/`metallic` numeric ones from the step definition, exactly as
/// draw.map always has: any string parameter other than `default_texture`
/// is treated as a mesh-name-pattern -> texture-name mapping.
DrawMapTextureConfig ReadDrawMapTextureConfig(
    const WorkflowStepDefinition& step);

/// True for texture names that mark a BSP portal surface (Q3's teleporter
/// shader convention: "*portal_sfx*" or "*mapobjects/portal*").
bool IsPortalTexture(const std::string& textureName);

/// The mapping pattern matching `meshName` wins; falls back to
/// `config.defaultTexture` when no pattern is found in the name.
std::string ResolveLegacyMeshTexture(const std::string& meshName,
                                     const DrawMapTextureConfig& config);

}  // namespace sdl3cpp::services::impl
