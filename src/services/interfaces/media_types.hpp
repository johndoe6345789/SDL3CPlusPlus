#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace sdl3cpp::services {

/**
 * Generic media item - represents a single media file (audio, video, image, etc.)
 * Can be used across any media catalog system (soundboard, music player, image browser, etc.)
 */
struct MediaItem {
    std::string id;                      // Unique identifier
    std::string label;                   // Display name
    std::filesystem::path path;          // File path
};

/**
 * Generic media category - represents a group of related media items
 * Can be used across any categorized media system
 */
struct MediaCategory {
    std::string id;                      // Unique identifier
    std::string name;                    // Display name
    std::filesystem::path basePath;      // Base directory for items
    std::vector<MediaItem> items;        // Items in this category
};

/**
 * Generic media catalog - represents a collection of categorized media
 * Parametric structure that works with any media system
 */
struct MediaCatalog {
    std::filesystem::path catalogRoot;   // Root directory for catalog
    std::vector<MediaCategory> categories; // Categories in catalog
};

/**
 * Generic media selection - represents user selection of a media item
 * Used by UI steps to communicate item selection to playback steps
 */
struct MediaSelection {
    bool hasSelection = false;           // Whether a selection was made
    std::uint64_t requestId = 0;         // Request ID for deduplication
    std::string categoryId;              // Selected category ID
    std::string itemId;                  // Selected item ID
    std::string label;                   // Item label
    std::filesystem::path path;          // Item path
};

}  // namespace sdl3cpp::services
