#include "racer_model_context.hpp"

namespace sdl3cpp::services::impl::racer_model_detail {
namespace {

// Mesh fields for collision (see the racer README).
constexpr std::uint32_t kFaceCount = 0x20;       // s16
constexpr std::uint32_t kPrimitive = 0x22;       // s16: 3, 4 or 5
constexpr std::uint32_t kFaceSizes = 0x24;       // s32 per face (strips)
constexpr std::uint32_t kCollisionVertices = 0x2C;
constexpr std::uint32_t kCollisionCount = 0x38;  // s16
constexpr std::uint32_t kPointBytes = 6;         // s16 x, y, z

constexpr std::int16_t kTriangles = 3;
constexpr std::int16_t kQuads = 4;
constexpr std::int16_t kStrips = 5;

struct Collision {
    ModelWalk& walk;
    std::uint32_t points;
    int count;
    const glm::mat4& transform;

    void Corner(int i) {
        const RacerBigEndianReader& r = walk.reader;
        const std::uint32_t at = points + kPointBytes * i;
        const glm::vec4 p = transform * glm::vec4(r.I16(at), r.I16(at + 2),
                                                  r.I16(at + 4), 1.f);
        walk.model.collision.insert(walk.model.collision.end(),
                                    {p.x, p.y, p.z});
    }

    void Triangle(int a, int b, int c) {
        if (a >= count || b >= count || c >= count) return;
        Corner(a);
        Corner(b);
        Corner(c);
    }
};

}  // namespace

void AppendCollision(ModelWalk& walk, std::uint32_t mesh,
                     const glm::mat4& transform) {
    const RacerBigEndianReader& r = walk.reader;
    const int count = r.I16(mesh + kCollisionCount);
    const std::uint32_t points = r.U32(mesh + kCollisionVertices);
    if (count < 3 || !walk.IsPointer(points, kPointBytes * count)) return;
    const int faces = r.I16(mesh + kFaceCount);
    const std::int16_t primitive = r.I16(mesh + kPrimitive);
    const std::uint32_t sizes = r.U32(mesh + kFaceSizes);
    Collision c{walk, points, count, transform};
    int first = 0;
    for (int face = 0; face < faces && first < count; ++face) {
        int corners = primitive;
        if (primitive == kStrips) {
            if (!walk.IsPointer(sizes, 4u * faces)) return;
            corners = r.I32(sizes + 4u * face);
        } else if (primitive != kTriangles && primitive != kQuads) {
            return;
        }
        // Quads are two triangles; strips one per corner after the second.
        for (int k = 0; k + 2 < corners; ++k) {
            c.Triangle(first + (primitive == kQuads ? 0 : k), first + k + 1,
                       first + k + 2);
        }
        first += corners;
    }
}

}  // namespace sdl3cpp::services::impl::racer_model_detail
