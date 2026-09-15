#pragma once

#include <zip.h>

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::q3 {

/**
 * @brief Read one entry out of a pk3 into memory.
 *
 * pk3s are zip archives and Quake addresses everything inside them by
 * path, so this is the single door between the archive and the engine.
 *
 * @return the entry's bytes, or empty when the archive or entry cannot
 *         be read; callers treat empty as "not available" rather than
 *         as an error, since content differs between pak files.
 */
std::vector<uint8_t> ReadPk3Entry(const std::string& pk3Path,
                                  const std::string& entryName);

/**
 * @brief Read one entry from an already-open pk3 archive.
 *
 * For callers reading many entries from the same pk3 (e.g. loading a
 * whole sound bank), reusing one zip_t* avoids reopening the archive
 * per entry the way ReadPk3Entry does.
 *
 * @return the entry's bytes, or empty when the entry cannot be read.
 */
std::vector<uint8_t> ReadPk3EntryFromArchive(zip_t* archive,
                                             const std::string& entryName);

}  // namespace sdl3cpp::q3
