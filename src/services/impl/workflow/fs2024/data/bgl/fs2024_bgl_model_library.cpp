#include "services/interfaces/workflow/fs2024/data/bgl/fs2024_bgl_model_library.hpp"

#include "services/interfaces/workflow/fs2024/data/bgl/fs2024_bgl_model_name.hpp"
#include "services/interfaces/workflow/fs2024/data/bgl/fs2024_bgl_stream_reader.hpp"

#include <algorithm>
#include <cstring>

namespace sdl3cpp::fs2024 {
namespace {

constexpr std::uint32_t kSectionModelData = 0x2B;
constexpr std::uint64_t kDirEntrySize = 24;
constexpr std::uint64_t kNamePeekSize = 1024;

void ReadDirectory(std::ifstream& in, std::uint64_t dataOffset,
                   std::uint32_t reccount,
                   std::vector<ModelLibraryEntry>& out) {
    std::vector<std::uint8_t> dir(reccount * kDirEntrySize);
    in.seekg(static_cast<std::streamoff>(dataOffset));
    in.read(reinterpret_cast<char*>(dir.data()),
           static_cast<std::streamsize>(dir.size()));
    if (!in) throw std::runtime_error("BGL model library: short directory");

    for (std::uint32_t i = 0; i < reccount; ++i) {
        const std::uint8_t* rec = dir.data() + i * kDirEntrySize;
        std::uint32_t relOffset = 0, size = 0;
        std::memcpy(&relOffset, rec + 16, 4);
        std::memcpy(&size, rec + 20, 4);

        ModelLibraryEntry entry;
        entry.guid = HexGuid(rec);
        entry.fileOffset = dataOffset + relOffset;
        entry.size = size;

        std::vector<std::uint8_t> peek(std::min<std::uint64_t>(
            kNamePeekSize, size));
        in.seekg(static_cast<std::streamoff>(entry.fileOffset));
        in.read(reinterpret_cast<char*>(peek.data()),
               static_cast<std::streamsize>(peek.size()));
        entry.name = NameFromGxmlPeek(peek);
        out.push_back(std::move(entry));
    }
}

}  // namespace

std::vector<ModelLibraryEntry> ListModelLibrary(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("BGL '" + path + "': cannot open");

    const auto sections = ReadPodAt<std::uint32_t>(in, 0x14);
    std::vector<ModelLibraryEntry> found;
    for (std::uint32_t i = 0; i < sections; ++i) {
        const std::uint64_t sectionAt = 0x38 + i * 20ull;
        if (ReadPodAt<std::uint32_t>(in, sectionAt) != kSectionModelData) {
            continue;
        }
        const auto subs = ReadPodAt<std::uint32_t>(in, sectionAt + 8);
        const auto tableOffset =
            ReadPodAt<std::uint32_t>(in, sectionAt + 12);
        for (std::uint32_t s = 0; s < subs; ++s) {
            const std::uint64_t subAt = tableOffset + s * 16ull;
            const auto reccount = ReadPodAt<std::uint32_t>(in, subAt + 4);
            const auto dataOffset = ReadPodAt<std::uint32_t>(in, subAt + 8);
            ReadDirectory(in, dataOffset, reccount, found);
        }
    }
    return found;
}

std::vector<std::uint8_t> ReadModelRiff(const std::string& path,
                                       const ModelLibraryEntry& entry) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("BGL '" + path + "': cannot open");
    std::vector<std::uint8_t> blob(entry.size);
    in.seekg(static_cast<std::streamoff>(entry.fileOffset));
    in.read(reinterpret_cast<char*>(blob.data()),
           static_cast<std::streamsize>(blob.size()));
    if (!in) throw std::runtime_error("BGL model library: short model read");
    return blob;
}

}  // namespace sdl3cpp::fs2024
