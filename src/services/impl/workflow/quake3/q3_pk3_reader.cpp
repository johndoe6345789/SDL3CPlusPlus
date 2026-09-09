#include "services/interfaces/workflow/quake3/q3_pk3_reader.hpp"

#include <zip.h>

namespace sdl3cpp::q3 {

std::vector<uint8_t> ReadPk3Entry(const std::string& pk3Path,
                                  const std::string& entryName) {
    if (pk3Path.empty() || entryName.empty()) {
        return {};
    }

    int err = 0;
    zip_t* archive = zip_open(pk3Path.c_str(), ZIP_RDONLY, &err);
    if (!archive) {
        return {};
    }

    std::vector<uint8_t> bytes;
    zip_stat_t stat;
    if (zip_stat(archive, entryName.c_str(), 0, &stat) == 0) {
        if (zip_file_t* file = zip_fopen(archive, entryName.c_str(), 0)) {
            bytes.resize(stat.size);
            zip_fread(file, bytes.data(), stat.size);
            zip_fclose(file);
        }
    }
    zip_close(archive);
    return bytes;
}

}  // namespace sdl3cpp::q3
