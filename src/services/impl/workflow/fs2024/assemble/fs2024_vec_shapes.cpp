#include "services/interfaces/workflow/fs2024/assemble/fs2024_vec_shapes.hpp"

#include "services/interfaces/workflow/fs2024/data/vec/fs2024_vec_frame.hpp"

#include <algorithm>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

namespace f = sdl3cpp::fs2024;

Fs2024VecShape Shape(const f::QuadTile& quad, const f::VecFeature& feature,
                     float left, float top, float tileSize) {
    Fs2024VecShape shape;
    shape.classBit = feature.classBit;
    shape.flags = feature.flags;
    shape.level = feature.level;
    for (const f::VecPoint& p : feature.points) {
        double fx = 0.0, fy = 0.0;
        f::VecPointInTile(quad, p, fx, fy);
        shape.points.push_back({left + static_cast<float>(fx) * tileSize,
                                top + static_cast<float>(fy) * tileSize});
    }
    return shape;
}

}  // namespace

Fs2024TileShapes GatherFs2024TileShapes(f::VecLibrary& vectors, int quadX,
                                        int quadY, int level,
                                        float tileSize) {
    Fs2024TileShapes shapes;
    const int side = 1 << (14 - level);
    shapes.side = side;
    shapes.quadSize = tileSize;
    shapes.mapped.assign(static_cast<std::size_t>(side) * side, false);
    const float span = tileSize * static_cast<float>(side);
    for (int dy = 0; dy < side; ++dy) {
        for (int dx = 0; dx < side; ++dx) {
            const f::QuadTile quad{quadX * side + dx, quadY * side + dy, 14};
            const f::VecTile vec = vectors.ReadTile(quad);
            shapes.mapped[static_cast<std::size_t>(dy) * side + dx] =
                !vec.roads.empty() || !vec.areas.empty() ||
                !vec.water.empty();
            const float left = static_cast<float>(dx) * tileSize;
            const float top = static_cast<float>(dy) * tileSize;
            for (const f::VecFeature& road : vec.roads) {
                shapes.roads.push_back(
                    Shape(quad, road, left, top, tileSize));
            }
            for (const f::VecFeature& water : vec.water) {
                Fs2024VecShape shape = Shape(quad, water, left, top,
                                             tileSize);
                shape.points = ClipRingToSquare(shape.points, span);
                if (shape.points.size() >= 3) {
                    shapes.water.push_back(std::move(shape));
                }
            }
        }
    }
    return shapes;
}

bool Fs2024TileShapes::Maps(float x, float z) const {
    if (side == 0) return false;
    const int qx = std::clamp(static_cast<int>(x / quadSize), 0, side - 1);
    const int qz = std::clamp(static_cast<int>(z / quadSize), 0, side - 1);
    return mapped[static_cast<std::size_t>(qz) * side + qx];
}

}  // namespace sdl3cpp::services::impl
