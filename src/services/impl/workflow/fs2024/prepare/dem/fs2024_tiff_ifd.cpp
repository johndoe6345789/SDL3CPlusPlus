#include "services/interfaces/workflow/fs2024/prepare/dem/fs2024_tiff_ifd.hpp"

#include <cstring>
#include <fstream>
#include <stdexcept>

namespace sdl3cpp::tools::fs2024 {
namespace {

// TIFF tag type sizes, indexed by the type code (1-12); 0 for unused.
constexpr std::uint32_t kTypeSize[] = {0, 1, 1, 2, 4, 8, 1, 1,
                                       2, 4, 8, 4, 8};

std::uint32_t TypeSize(std::uint16_t type) {
    return type < std::size(kTypeSize) ? kTypeSize[type] : 0;
}

/// Little-endian TIFFs only: every GeoTIFF DEM this reads is 'II'.
/// (Copernicus GLO-30 ships nothing else, and a project this narrow
/// gains nothing from supporting the 'MM' byte order it never sees.)
template <typename T>
T ReadLE(const std::uint8_t* p) {
    T value{};
    std::memcpy(&value, p, sizeof(T));
    return value;
}

}  // namespace

std::uint32_t TiffTag::AsUint32(std::size_t index) const {
    const std::uint32_t size = TypeSize(type);
    if (size == 0 || (index + 1) * size > raw.size()) return 0;
    const std::uint8_t* p = raw.data() + index * size;
    if (size == 1) return *p;
    if (size == 2) return ReadLE<std::uint16_t>(p);
    return ReadLE<std::uint32_t>(p);
}

double TiffTag::AsDouble(std::size_t index) const {
    if ((index + 1) * 8 > raw.size()) return 0.0;
    return ReadLE<double>(raw.data() + index * 8);
}

std::vector<std::uint32_t> TiffTag::AsUint32Array() const {
    std::vector<std::uint32_t> values;
    values.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) values.push_back(AsUint32(i));
    return values;
}

namespace {

[[noreturn]] void Fail(const std::string& path, const std::string& why) {
    throw std::runtime_error("GeoTIFF '" + path + "': " + why);
}

TiffTag ReadTag(std::ifstream& in, const std::string& path,
               const std::uint8_t entry[12]) {
    TiffTag tag;
    tag.id = ReadLE<std::uint16_t>(entry);
    tag.type = ReadLE<std::uint16_t>(entry + 2);
    tag.count = ReadLE<std::uint32_t>(entry + 4);
    const std::uint32_t bytes = TypeSize(tag.type) * tag.count;
    if (bytes == 0) Fail(path, "tag with an unknown type");

    tag.raw.resize(bytes);
    if (bytes <= 4) {
        std::memcpy(tag.raw.data(), entry + 8, bytes);
    } else {
        const auto offset = ReadLE<std::uint32_t>(entry + 8);
        in.seekg(offset);
        in.read(reinterpret_cast<char*>(tag.raw.data()),
               static_cast<std::streamsize>(bytes));
        if (!in) Fail(path, "tag value runs past the end of the file");
    }
    return tag;
}

}  // namespace

TiffDirectory ReadTiffDirectory(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) Fail(path, "cannot open");

    std::uint8_t header[8];
    in.read(reinterpret_cast<char*>(header), 8);
    if (!in || header[0] != 'I' || header[1] != 'I' ||
        ReadLE<std::uint16_t>(header + 2) != 42) {
        Fail(path, "not a little-endian TIFF");
    }
    in.seekg(ReadLE<std::uint32_t>(header + 4));

    std::uint8_t countBytes[2];
    in.read(reinterpret_cast<char*>(countBytes), 2);
    const auto entryCount = ReadLE<std::uint16_t>(countBytes);

    // Read the whole entry table in one sequential pass first: ReadTag
    // below seeks elsewhere in the file for any value over 4 bytes, so
    // walking the table via repeated read-then-seek-back left "seek
    // back" computed from nothing but the loop index, not this IFD's
    // real file offset -- garbage entries past the first one, and the
    // stack-buffer overrun that comes from trusting their sizes.
    std::vector<std::uint8_t> table(static_cast<std::size_t>(entryCount) *
                                    12);
    in.read(reinterpret_cast<char*>(table.data()),
           static_cast<std::streamsize>(table.size()));
    if (!in) Fail(path, "IFD entry table runs past the end of the file");

    TiffDirectory dir;
    for (std::uint16_t i = 0; i < entryCount; ++i) {
        const TiffTag tag = ReadTag(in, path, table.data() + i * 12);
        switch (tag.id) {
            case 256: dir.width = tag.AsUint32(); break;
            case 257: dir.height = tag.AsUint32(); break;
            case 258:
                dir.bitsPerSample =
                    static_cast<std::uint16_t>(tag.AsUint32());
                break;
            case 259:
                dir.compression = static_cast<std::uint16_t>(tag.AsUint32());
                break;
            case 317:
                dir.predictor = static_cast<std::uint16_t>(tag.AsUint32());
                break;
            case 322: dir.tileWidth = tag.AsUint32(); break;
            case 323: dir.tileLength = tag.AsUint32(); break;
            case 324: dir.tileOffsets = tag.AsUint32Array(); break;
            case 325: dir.tileByteCounts = tag.AsUint32Array(); break;
            case 339:
                dir.sampleFormat = static_cast<std::uint16_t>(tag.AsUint32());
                break;
            case 33550:
                dir.pixelScaleX = tag.AsDouble(0);
                dir.pixelScaleY = tag.AsDouble(1);
                break;
            case 33922:
                dir.tiepointLon = tag.AsDouble(3);
                dir.tiepointLat = tag.AsDouble(4);
                break;
            default: break;
        }
    }
    if (dir.tileOffsets.empty()) Fail(path, "not tiled (unsupported)");
    return dir;
}

}  // namespace sdl3cpp::tools::fs2024
