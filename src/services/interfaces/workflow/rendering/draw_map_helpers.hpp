#pragma once

#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

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

/// Builds the vertex uniforms shared by every draw call this frame: MVP for
/// an identity model matrix, camera position, and the shadow-pass VP. The
/// normal starts as +Y; legacy (non-BSP) mode overwrites it per mesh.
rendering::VertexUniformData BuildDrawMapVertexUniforms(
    const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos,
    const glm::mat4& shadowVP);

/**
 * @brief Draws every BSP texture group in `mapNodes` with one shared VB/IB.
 *
 * Binds the single vertex/index buffer once, then for each node binds that
 * group's albedo (falling back to `defaultTexture`) plus shadow/lightmap/
 * portal-destination samplers, and issues one indexed draw call.
 */
void DrawBspMapGeometry(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                        WorkflowContext& context,
                        const nlohmann::json& mapNodes,
                        const rendering::FragmentUniformData& fu,
                        const rendering::VertexUniformData& vu,
                        const std::string& defaultTexture);

/**
 * @brief Draws every legacy (non-BSP) mesh in `mapNodes`, one draw call each.
 *
 * Each mesh has its own VB/IB with 16-bit indices; the mesh's texture is
 * whichever mapping's pattern is found in the mesh name (or `defaultTexture`
 * if none match), and its normal is derived from the thinnest axis of its
 * bounding box when present.
 */
void DrawLegacyMapGeometry(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                           WorkflowContext& context,
                           const nlohmann::json& mapNodes,
                           rendering::VertexUniformData vu,
                           const rendering::FragmentUniformData& fu,
                           const DrawMapTextureConfig& config);

}  // namespace sdl3cpp::services::impl
