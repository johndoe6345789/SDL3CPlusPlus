#pragma once

#include "services/interfaces/media_types.hpp"

#include <rapidjson/document.h>

#include <filesystem>
#include <vector>

namespace sdl3cpp::services::impl {

/// Scans `directory` for regular files and returns one MediaItem per file,
/// sorted case-insensitively by filename. Returns an empty vector if
/// `directory` does not exist.
std::vector<MediaItem> LoadMediaItems(const std::filesystem::path& directory);

/**
 * @brief Builds a MediaCatalog from a parsed catalog JSON document.
 *
 * Expects a top-level "categories" array of objects, each with string
 * "id", "name", and "path" fields; `path` is resolved relative to
 * `packageRoot` and scanned via LoadMediaItems.
 *
 * @throws std::runtime_error if "categories" is missing/not an array, or
 *         any category object is malformed.
 */
MediaCatalog BuildMediaCatalog(const rapidjson::Document& document,
                               const std::filesystem::path& packageRoot);

}  // namespace sdl3cpp::services::impl
