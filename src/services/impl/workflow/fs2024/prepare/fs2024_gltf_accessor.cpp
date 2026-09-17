#include "services/interfaces/workflow/fs2024/prepare/fs2024_gltf_accessor.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <nlohmann/json.hpp>

namespace sdl3cpp::tools::fs2024 {
namespace {
using nlohmann::json;

int ComponentSize(int componentType) {
    switch (componentType) {
        case 5120: case 5121: return 1;  // (UNSIGNED_)BYTE
        case 5122: case 5123: return 2;  // (UNSIGNED_)SHORT
        default: return 4;               // UNSIGNED_INT, FLOAT
    }
}

int TypeComponents(const std::string& type) {
    if (type == "SCALAR") return 1;
    if (type == "VEC2") return 2;
    if (type == "VEC3") return 3;
    return 4;  // VEC4
}

/// The base pointer and per-element stride of an accessor's data, and
/// its declared component type/count for the caller to read with.
const std::uint8_t* AccessorBase(const json& gltf, const std::uint8_t* bin,
                                 int accessorIndex, int& stride,
                                 int& componentType, int& count) {
    const auto& acc = gltf.at("accessors").at(accessorIndex);
    const auto& bv =
        gltf.at("bufferViews").at(acc.at("bufferView").get<int>());
    componentType = acc.at("componentType").get<int>();
    count = acc.at("count").get<int>();
    const int tight =
        ComponentSize(componentType) *
        TypeComponents(acc.at("type").get<std::string>());
    stride = bv.value("byteStride", 0);
    if (stride == 0) stride = tight;
    return bin + bv.value("byteOffset", 0) + acc.value("byteOffset", 0);
}

/// FS2024's own exporter ("ASOBO_asset_optimized") quantises NORMAL,
/// TANGENT and TEXCOORD as signed-normalised BYTE/SHORT rather than
/// FLOAT -- without ever declaring the accessor `"normalized": true`
/// the core glTF spec would require for that to be valid, so a
/// spec-following reader has no way to know from the JSON alone.
/// Read once as `int8_t`/`int16_t` per the standard KHR_mesh_quantization
/// formula (`max(v / 127.0, -1.0)`, `max(v / 32767.0, -1.0)`); FLOAT is
/// read as-is.
float ReadComponent(const std::uint8_t* p, int componentType) {
    switch (componentType) {
        case 5120: {  // BYTE
            const auto v = static_cast<std::int8_t>(p[0]);
            return std::max(static_cast<float>(v) / 127.f, -1.f);
        }
        case 5122: {  // SHORT
            std::int16_t v = 0;
            std::memcpy(&v, p, 2);
            return std::max(static_cast<float>(v) / 32767.f, -1.f);
        }
        default: {  // FLOAT
            float v = 0.f;
            std::memcpy(&v, p, 4);
            return v;
        }
    }
}

}  // namespace

std::vector<glm::vec3> ReadFloat3Accessor(const json& gltf,
                                         const std::uint8_t* bin,
                                         int accessorIndex) {
    int stride = 0, componentType = 0, count = 0;
    const auto* base =
        AccessorBase(gltf, bin, accessorIndex, stride, componentType, count);
    const int componentSize = ComponentSize(componentType);
    std::vector<glm::vec3> out(count);
    for (int i = 0; i < count; ++i) {
        const std::uint8_t* element = base + i * stride;
        for (int c = 0; c < 3; ++c) {
            out[i][c] = ReadComponent(element + c * componentSize,
                                     componentType);
        }
    }
    return out;
}

std::vector<glm::vec2> ReadFloat2Accessor(const json& gltf,
                                         const std::uint8_t* bin,
                                         int accessorIndex) {
    int stride = 0, componentType = 0, count = 0;
    const auto* base =
        AccessorBase(gltf, bin, accessorIndex, stride, componentType, count);
    const int componentSize = ComponentSize(componentType);
    std::vector<glm::vec2> out(count);
    for (int i = 0; i < count; ++i) {
        const std::uint8_t* element = base + i * stride;
        for (int c = 0; c < 2; ++c) {
            out[i][c] = ReadComponent(element + c * componentSize,
                                     componentType);
        }
    }
    return out;
}

std::vector<std::uint32_t> ReadIndexAccessor(const json& gltf,
                                            const std::uint8_t* bin,
                                            int accessorIndex) {
    int stride = 0, componentType = 0, count = 0;
    const auto* base =
        AccessorBase(gltf, bin, accessorIndex, stride, componentType, count);
    std::vector<std::uint32_t> out(count);
    for (int i = 0; i < count; ++i) {
        const std::uint8_t* p = base + i * stride;
        if (componentType == 5121) {
            out[i] = p[0];
        } else if (componentType == 5123) {
            std::uint16_t v = 0;
            std::memcpy(&v, p, 2);
            out[i] = v;
        } else {
            std::memcpy(&out[i], p, 4);
        }
    }
    return out;
}

}  // namespace sdl3cpp::tools::fs2024
