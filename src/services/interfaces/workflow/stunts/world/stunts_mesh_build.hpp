#pragma once

#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"
#include "services/interfaces/workflow/stunts/data/stunts_tile_table.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Appends one quad, given its four corners in winding order and the
/// number of times the texture repeats along the v axis.
void AppendStuntsQuad(GeometryPlaneMesh& mesh, const glm::vec3& a,
                      const glm::vec3& b, const glm::vec3& c,
                      const glm::vec3& d, float vRepeat);

/// Unit vector pointing out of the cell through `link`.
glm::vec3 StuntsLinkDirection(std::uint8_t link);

/// Appends the road surface of one straight or junction arm: a strip
/// from the cell centre out to the edge `link` opens onto.
void AppendStuntsArm(GeometryPlaneMesh& mesh, const glm::vec3& centre,
                     std::uint8_t link, float tileSize, float width);

/// Appends a quarter turn joining the two edges a corner opens onto.
void AppendStuntsCorner(GeometryPlaneMesh& mesh, const glm::vec3& centre,
                        std::uint8_t links, float tileSize, float width,
                        int segments);

}  // namespace sdl3cpp::services::impl
