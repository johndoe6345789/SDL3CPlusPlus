#ifndef SDL3CPP_CORE_MATH_HPP
#define SDL3CPP_CORE_MATH_HPP

#include <array>
#include <cmath>
#include <cstdint>

namespace sdl3cpp::core {

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Vertex {
    std::array<float, 3> position;
    std::array<float, 3> color;
};

struct PushConstants {
    std::array<float, 16> model;
    std::array<float, 16> viewProj;
};

static_assert(sizeof(PushConstants) == sizeof(float) * 32, "push constant size mismatch");

inline std::array<float, 16> MultiplyMatrix(const std::array<float, 16>& a,
                                            const std::array<float, 16>& b) {
    std::array<float, 16> result{};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            float sum = 0.0f;
            for (int idx = 0; idx < 4; ++idx) {
                sum += a[idx * 4 + row] * b[col * 4 + idx];
            }
            result[col * 4 + row] = sum;
        }
    }
    return result;
}

inline std::array<float, 16> IdentityMatrix() {
    return {1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f};
}

inline Vec3 Normalize(Vec3 v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len == 0.0f) {
        return v;
    }
    return {v.x / len, v.y / len, v.z / len};
}

inline Vec3 Cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

inline float Dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline std::array<float, 16> LookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    Vec3 f = Normalize({center.x - eye.x, center.y - eye.y, center.z - eye.z});
    Vec3 s = Normalize(Cross(f, up));
    Vec3 u = Cross(s, f);

    std::array<float, 16> result = IdentityMatrix();
    result[0] = s.x;
    result[1] = u.x;
    result[2] = -f.x;
    result[4] = s.y;
    result[5] = u.y;
    result[6] = -f.y;
    result[8] = s.z;
    result[9] = u.z;
    result[10] = -f.z;
    result[12] = -Dot(s, eye);
    result[13] = -Dot(u, eye);
    result[14] = Dot(f, eye);
    return result;
}

inline std::array<float, 16> Perspective(float fovRadians, float aspect, float zNear, float zFar) {
    float tanHalf = std::tan(fovRadians / 2.0f);
    std::array<float, 16> result{};
    result[0] = 1.0f / (aspect * tanHalf);
    result[5] = -1.0f / tanHalf;
    result[10] = zFar / (zNear - zFar);
    result[11] = -1.0f;
    result[14] = (zNear * zFar) / (zNear - zFar);
    return result;
}

} // namespace sdl3cpp::core

#endif // SDL3CPP_CORE_MATH_HPP
