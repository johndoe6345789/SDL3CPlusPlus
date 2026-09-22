#pragma once

#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"
#include "services/interfaces/workflow/stunts/data/stunts_shape.hpp"

namespace sdl3cpp::services::impl {

/**
 * @brief Game units to metres, calibrated against a real car.
 *
 * The files carry no unit. This was found by comparing the Acura
 * NSX's decoded body shape to the real car's length and width (its
 * two least ambiguous dimensions): both agree to within 2% at this
 * factor. Applies to every shape -- car bodies, wheels and track
 * pieces share one coordinate space.
 */
constexpr float kStuntsUnitsToMetres = 0.00224f;

/// Rows in the palette texture stunts_materials.json is baked to;
/// samples land safely inside a row regardless of filtering.
constexpr float kStuntsPaletteRows = 4.f;

/// The palette-texture V coordinate a solid-colour vertex samples at.
constexpr float kStuntsPaletteV = 0.5f / kStuntsPaletteRows;

/**
 * @brief Builds a drawable mesh from a shape's Polygon faces.
 *
 * Each face is fan-triangulated from its first vertex. Wheel
 * primitives are skipped here -- see stunts_wheel_mesh.hpp -- since
 * they are a different kind of geometry entirely, not an odd polygon.
 *
 * `paintJob` selects which of the shape's material columns to sample;
 * out-of-range values clamp to the last one. Every vertex of a face
 * gets that face's own colour by sampling the palette texture
 * (packages/stunts/assets/stunts_palette.png, built by
 * packages/stunts/tools from stunts_materials.json) at
 * `(materialId + 0.5) / 256`.
 */
GeometryPlaneMesh BuildStuntsShapeMesh(const StuntsShape& shape,
                                       std::size_t paintJob = 0);

}  // namespace sdl3cpp::services::impl
