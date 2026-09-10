#include "services/interfaces/workflow/quake3/q3_pk3_reader.hpp"

namespace sdl3cpp::q3 {

std::vector<uint8_t> ReadPk3EntryFromArchive(zip_t* archive,
                                             const std::string& entryName) {
    if (!archive || entryName.empty()) {
        return {};
    }

    // Quake3's own .skin files reference paths with different case than
    // the pk3 entries actually use (e.g. "players/Keel/keel.tga" vs the
    // archive's "players/keel/keel.tga"); ioquake3 resolves pk3 paths
    // case-insensitively, so match that here rather than losing textures
    // to a case-sensitive lookup.
    std::vector<uint8_t> bytes;
    zip_stat_t stat;
    if (zip_stat(archive, entryName.c_str(), ZIP_FL_NOCASE, &stat) == 0) {
        if (zip_file_t* file =
                zip_fopen(archive, entryName.c_str(), ZIP_FL_NOCASE)) {
            bytes.resize(stat.size);
            zip_fread(file, bytes.data(), stat.size);
            zip_fclose(file);
        }
    }
    return bytes;
}

std::vector<uint8_t> ReadPk3Entry(const std::string& pk3Path,
                                  const std::string& entryName) {
    if (pk3Path.empty() || entryName.empty()) {
        return {};
    }

    int err        = 0;
    zip_t* archive = zip_open(pk3Path.c_str(), ZIP_RDONLY, &err);
    if (!archive) {
        return {};
    }

    std::vector<uint8_t> bytes = ReadPk3EntryFromArchive(archive, entryName);
    zip_close(archive);
    return bytes;
}

}  // namespace sdl3cpp::q3
