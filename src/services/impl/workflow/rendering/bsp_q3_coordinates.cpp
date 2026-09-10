#include "services/interfaces/workflow/rendering/bsp_q3_coordinates.hpp"

#include <algorithm>
#include <limits>

namespace sdl3cpp::services::impl {

std::array<float, 3> ConvertQ3Point(float qx, float qy, float qz,
                                    float scale) {
    return {qx * scale, qz * scale, -qy * scale};
}

nlohmann::json PointJson(const std::array<float, 3>& p) {
    return nlohmann::json::array({p[0], p[1], p[2]});
}

nlohmann::json ConvertModelBounds(const BspModel& model, float scale) {
    std::array<float, 3> mn{std::numeric_limits<float>::max(),
                            std::numeric_limits<float>::max(),
                            std::numeric_limits<float>::max()};
    std::array<float, 3> mx{std::numeric_limits<float>::lowest(),
                            std::numeric_limits<float>::lowest(),
                            std::numeric_limits<float>::lowest()};

    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            for (int z = 0; z < 2; ++z) {
                const auto p =
                    ConvertQ3Point(x ? model.maxs[0] : model.mins[0],
                                   y ? model.maxs[1] : model.mins[1],
                                   z ? model.maxs[2] : model.mins[2], scale);
                for (int axis = 0; axis < 3; ++axis) {
                    mn[axis] = std::min(mn[axis], p[axis]);
                    mx[axis] = std::max(mx[axis], p[axis]);
                }
            }
        }
    }
    return nlohmann::json{{"min", PointJson(mn)}, {"max", PointJson(mx)}};
}

}  // namespace sdl3cpp::services::impl
