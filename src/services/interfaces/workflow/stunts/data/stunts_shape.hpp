#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// One shape vertex, as stored: signed 16-bit game units.
struct StuntsShapeVertex {
    std::int16_t x = 0;
    std::int16_t y = 0;
    std::int16_t z = 0;
};

/// What a primitive's payload means: a flat polygon's vertex indices,
/// or a wheel's fixed six-byte parameter block.
enum class StuntsPrimitiveKind : std::uint8_t { Polygon, Wheel };

/**
 * @brief One primitive: a flat 2-10 sided polygon, or a wheel.
 *
 * `materials` holds one colour id per paint job the shape offers --
 * the same slot across every primitive selects one consistent colour
 * scheme. A part that does not change between schemes (tyres, glass)
 * simply repeats the same id in every slot, which is how this was
 * told apart from a shorter, single-material encoding: there never
 * is one -- every primitive carries exactly as many material bytes
 * as the shape has paint jobs.
 *
 * For a Polygon, `indices` are into the shape's vertex list, one per
 * side. For a Wheel, `payload` is that primitive's six raw bytes --
 * their layout is not decoded yet, so they are kept for later use
 * rather than dropped.
 */
struct StuntsShapeFace {
    StuntsPrimitiveKind kind = StuntsPrimitiveKind::Polygon;
    std::uint8_t flags = 0;
    std::vector<std::uint8_t> materials;
    std::vector<std::uint8_t> indices;   // Polygon only
    std::array<std::uint8_t, 6> payload{};  // Wheel only
};

/// One decoded 3D shape: a vertex list and its faces.
struct StuntsShape {
    std::vector<StuntsShapeVertex> vertices;
    std::vector<StuntsShapeFace> faces;
    std::uint8_t paintJobs = 0;
    bool valid = false;
};

/**
 * @brief Parses one shape's bytes, exactly as stunts.world.load reads
 *        one entry out of a shape archive.
 *
 * The header is `numVertices`, `numPrimitives`, `numPaintJobs`, then
 * a reserved byte that is always 0 in every file this was checked
 * against. The vertex list and an 8-byte-per-primitive culling block
 * follow, then the primitives themselves.
 *
 * A primitive is `[type][flags][materials * numPaintJobs][payload]`.
 * Type 1-10 is a polygon with that many sides, and its payload is
 * that many vertex indices; type 11-13 is a wheel, whose payload is
 * a fixed six bytes regardless of the type value. This was confirmed
 * against every shape in a retail install: every polygon index stays
 * inside the vertex list, and every shape's primitives run out with
 * only the small paint-job trailer left over.
 *
 * @return An invalid (`valid = false`) shape if the bytes do not fit
 *         this layout exactly.
 */
StuntsShape ParseStuntsShape(const std::vector<std::uint8_t>& data);

}  // namespace sdl3cpp::services::impl
