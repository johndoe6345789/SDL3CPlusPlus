#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Matches VertexUniforms in fs2024_terrain.vert.
struct Fs2024TerrainVertexUniforms {
    glm::mat4 viewProj{1.f};
    glm::vec4 cameraPos{0.f};
    glm::vec4 originOffset{0.f};  ///< xyz: the tile's local origin
};

/// Matches FragmentUniforms in fs2024_terrain.frag. Colours are linear.
struct Fs2024TerrainFragmentUniforms {
    glm::vec4 sunDir{0.f, -1.f, 0.f, 0.f};  ///< direction light travels
    glm::vec4 sunColour{1.f};
    glm::vec4 ambient{0.4f, 0.45f, 0.55f, 1.f};
    glm::vec4 fog{0.55f, 0.6f, 0.7f, 0.00006f};  ///< rgb, density per m
    /// Runway painted procedurally: centre x, z, half length, half width.
    /// A zero half length means no runway.
    glm::vec4 runway{0.f};
    /// Unit vector down the runway in x, z (heading's direction); z, w 0.
    glm::vec4 runwayAxis{0.f};
};

Fs2024TerrainVertexUniforms BuildFs2024TerrainVertexUniforms(
    const WorkflowContext& context);

/// Sun and ambient from lighting.setup; fog colour from the sky's
/// horizon when gta5.sky.draw has run, so the ground fades into it. The
/// runway from runway_x, runway_z, runway_heading, runway_length and
/// runway_width, when runway_length is given.
Fs2024TerrainFragmentUniforms BuildFs2024TerrainFragmentUniforms(
    const WorkflowStepDefinition& step, const WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
