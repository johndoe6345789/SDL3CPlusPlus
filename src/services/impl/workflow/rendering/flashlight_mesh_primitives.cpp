#include "services/interfaces/workflow/rendering/flashlight_mesh_internal.hpp"

#include <cmath>

namespace sdl3cpp::services::impl::flashlight_mesh_detail {

namespace {
constexpr float kPi = 3.14159265358979f;
}  // namespace

void AddCylinder(std::vector<PosUvVertex>& vertices,
                 std::vector<uint16_t>& indices, int segments, float r1,
                 float r2, float y_start, float y_end, float uv_start,
                 float uv_end) {
    uint16_t base = static_cast<uint16_t>(vertices.size());

    for (int i = 0; i <= segments; ++i) {
        float angle = (static_cast<float>(i) / segments) * 2.0f * kPi;
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);
        float u     = static_cast<float>(i) / segments;

        vertices.push_back({cos_a * r1, y_start, sin_a * r1, u, uv_start});
        vertices.push_back({cos_a * r2, y_end, sin_a * r2, u, uv_end});
    }

    for (int i = 0; i < segments; ++i) {
        uint16_t b = base + static_cast<uint16_t>(i * 2);
        indices.push_back(b);
        indices.push_back(b + 1);
        indices.push_back(b + 2);
        indices.push_back(b + 2);
        indices.push_back(b + 1);
        indices.push_back(b + 3);
    }
}

void AddCap(std::vector<PosUvVertex>& vertices, std::vector<uint16_t>& indices,
            int segments, float radius, float y, float uv_v, bool flip) {
    uint16_t center = static_cast<uint16_t>(vertices.size());
    vertices.push_back({0.0f, y, 0.0f, 0.5f, uv_v});

    for (int i = 0; i <= segments; ++i) {
        float angle = (static_cast<float>(i) / segments) * 2.0f * kPi;
        vertices.push_back({std::cos(angle) * radius, y,
                            std::sin(angle) * radius,
                            0.5f + 0.5f * std::cos(angle), uv_v});
    }

    for (int i = 0; i < segments; ++i) {
        uint16_t a = center + 1 + static_cast<uint16_t>(i);
        uint16_t b = a + 1;
        if (flip) {
            indices.push_back(center);
            indices.push_back(b);
            indices.push_back(a);
        } else {
            indices.push_back(center);
            indices.push_back(a);
            indices.push_back(b);
        }
    }
}

}  // namespace sdl3cpp::services::impl::flashlight_mesh_detail
