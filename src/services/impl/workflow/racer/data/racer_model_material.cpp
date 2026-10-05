#include "racer_model_context.hpp"

namespace sdl3cpp::services::impl::racer_model_detail {
namespace {

// MeshMaterial: flags, uv offsets, then a MaterialTexture pointer.
constexpr std::uint32_t kMeshMaterialTexture = 0x08;
// MaterialTexture (0x40 bytes).
constexpr std::uint32_t kTextureBytes = 0x40;
constexpr std::uint32_t kFormat = 0x0C;
constexpr std::uint32_t kWidth = 0x10;
constexpr std::uint32_t kHeight = 0x12;
constexpr std::uint32_t kChildren = 0x1C;   // six pointers
constexpr std::uint32_t kTextureIndex = 0x38;
// MaterialTextureChild byte 3: the dimensions bitmask.
constexpr std::uint8_t kDoubleHeight = 0x01;
constexpr std::uint8_t kDoubleWidth = 0x10;

bool KnownFormat(std::uint16_t format) {
    switch (static_cast<RacerTextureFormat>(format)) {
    case RacerTextureFormat::Rgba32:
    case RacerTextureFormat::Indexed4:
    case RacerTextureFormat::Indexed8:
    case RacerTextureFormat::Intensity4:
    case RacerTextureFormat::Intensity8:
        return true;
    }
    return false;
}

}  // namespace

RacerMaterialRef ReadMaterial(const ModelWalk& walk, std::uint32_t offset) {
    const RacerBigEndianReader& r = walk.reader;
    RacerMaterialRef ref;
    if (!walk.IsPointer(offset, 0x10)) return ref;
    const std::uint32_t texture = r.U32(offset + kMeshMaterialTexture);
    if (!walk.IsPointer(texture, kTextureBytes)) return ref;

    // The index word carries a 0x0A signature in its top byte; the low
    // 24 bits are the texture block index, all ones meaning none.
    const std::uint32_t index = r.U32(texture + kTextureIndex) & 0xFFFFFF;
    const std::uint16_t format = r.U16(texture + kFormat);
    if (index == 0xFFFFFF || !KnownFormat(format)) return ref;
    ref.textureIndex = static_cast<int>(index);
    ref.format = static_cast<RacerTextureFormat>(format);
    ref.width = r.I16(texture + kWidth);
    ref.height = r.I16(texture + kHeight);

    // The first child that exists says whether the image is mirrored.
    for (std::uint32_t k = 0; k < 6; ++k) {
        const std::uint32_t child = r.U32(texture + kChildren + 4 * k);
        if (!walk.IsPointer(child, 16)) continue;
        const std::uint8_t bits = r.U8(child + 3);
        ref.doubleWidth = (bits & kDoubleWidth) != 0;
        ref.doubleHeight = (bits & kDoubleHeight) != 0;
        break;
    }
    return ref;
}

}  // namespace sdl3cpp::services::impl::racer_model_detail
