#pragma once

#include "services/interfaces/workflow/rendering/draw_textured_transform.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"

namespace sdl3cpp::services::impl {

/// Fills the vertex/fragment uniforms shared by this draw call from the
/// context's pre-computed camera/shadow/lighting state plus this call's
/// model matrix, normal, and material parameters.
void BuildDrawTexturedUniforms(const WorkflowContext& context,
                               const DrawTexturedTransform& transform,
                               float roughness, float metallic,
                               rendering::VertexUniformData& vu,
                               rendering::FragmentUniformData& fu);

}  // namespace sdl3cpp::services::impl
