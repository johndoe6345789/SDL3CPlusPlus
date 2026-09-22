#include "services/interfaces/workflow/stunts/data/stunts_shape.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr std::size_t kHeaderBytes = 4;
constexpr std::size_t kVertexBytes = 6;
constexpr std::size_t kCullBytesPerPrim = 8;
constexpr std::size_t kMaxTrailer = 8;
constexpr std::uint8_t kMaxPolygonSides = 10;
constexpr std::uint8_t kMaxWheelType = 13;
constexpr std::size_t kWheelPayloadBytes = 6;

std::int16_t ReadI16(const std::vector<std::uint8_t>& d, std::size_t at) {
    return static_cast<std::int16_t>(d[at] | (d[at + 1] << 8));
}

std::vector<StuntsShapeVertex> ReadVertices(
    const std::vector<std::uint8_t>& data, std::size_t at,
    std::size_t count) {
    std::vector<StuntsShapeVertex> vertices;
    vertices.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t base = at + i * kVertexBytes;
        vertices.push_back({ReadI16(data, base), ReadI16(data, base + 2),
                            ReadI16(data, base + 4)});
    }
    return vertices;
}

/// Reads one primitive at `at`; advances `at` past it on success.
bool ReadFace(const std::vector<std::uint8_t>& data, std::size_t& at,
             std::size_t vertexCount, std::size_t paintJobs,
             StuntsShapeFace& face) {
    if (at >= data.size()) return false;
    const std::uint8_t type = data[at];
    const bool isWheel = type > kMaxPolygonSides;
    if (type < 1 || type > kMaxWheelType) return false;
    const std::size_t payloadLen = isWheel ? kWheelPayloadBytes : type;
    const std::size_t width = 2 + paintJobs + payloadLen;
    if (at + width > data.size()) return false;

    face.kind = isWheel ? StuntsPrimitiveKind::Wheel
                        : StuntsPrimitiveKind::Polygon;
    face.flags = data[at + 1];
    face.materials.assign(data.begin() + static_cast<long>(at + 2),
                          data.begin() + static_cast<long>(at + 2 + paintJobs));
    const std::size_t payloadAt = at + 2 + paintJobs;
    if (isWheel) {
        std::copy_n(data.begin() + static_cast<long>(payloadAt),
                   kWheelPayloadBytes, face.payload.begin());
    } else {
        for (std::size_t i = 0; i < payloadLen; ++i) {
            const std::uint8_t index = data[payloadAt + i];
            if (index >= vertexCount) return false;
        }
        face.indices.assign(
            data.begin() + static_cast<long>(payloadAt),
            data.begin() + static_cast<long>(payloadAt + payloadLen));
    }
    at += width;
    return true;
}

}  // namespace

StuntsShape ParseStuntsShape(const std::vector<std::uint8_t>& data) {
    StuntsShape shape;
    if (data.size() < kHeaderBytes) return shape;
    const std::size_t numVertices = data[0];
    const std::size_t numPrimitives = data[1];
    const std::size_t numPaintJobs = data[2];

    const std::size_t vertexAt = kHeaderBytes;
    const std::size_t vertexEnd = vertexAt + numVertices * kVertexBytes;
    const std::size_t primAt = vertexEnd + numPrimitives * kCullBytesPerPrim;
    if (primAt > data.size()) return shape;

    std::vector<StuntsShapeFace> faces;
    faces.reserve(numPrimitives);
    std::size_t at = primAt;
    for (std::size_t i = 0; i < numPrimitives; ++i) {
        StuntsShapeFace face;
        if (!ReadFace(data, at, numVertices, numPaintJobs, face)) return shape;
        faces.push_back(std::move(face));
    }
    if (data.size() - at > kMaxTrailer) return shape;

    shape.vertices = ReadVertices(data, vertexAt, numVertices);
    shape.faces = std::move(faces);
    shape.paintJobs = static_cast<std::uint8_t>(numPaintJobs);
    shape.valid = true;
    return shape;
}

}  // namespace sdl3cpp::services::impl
