#pragma once

#include "services/interfaces/workflow/rendering/rendering_types.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/**
 * @brief Vertex uniforms placing the sky dome around the camera.
 *
 * The view's translation is dropped so the dome cannot be walked
 * towards. The clouds move by scrolling their texture coordinates (see
 * UpdateSkyScroll), not by turning the dome.
 */
rendering::VertexUniformData BuildSkyVertexUniforms(const glm::mat4& view,
                                                    const glm::mat4& proj);

/// Fragment uniforms that render the cloud texture unlit: no ambient,
/// exposure 1, and a lightmap overbright of 1 against a white lightmap.
rendering::FragmentUniformData BuildSkyFragmentUniforms();

}  // namespace sdl3cpp::services::impl
