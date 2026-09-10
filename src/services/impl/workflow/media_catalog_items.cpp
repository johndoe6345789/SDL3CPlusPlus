#include "services/interfaces/workflow/media_catalog_builder.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

std::string ToLower(std::string value) {
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string PrettyItemName(const std::string& fileName) {
    std::string base = fileName;
    const auto dot   = base.find_last_of('.');
    if (dot != std::string::npos) {
        base = base.substr(0, dot);
    }
    for (char& ch : base) {
        if (ch == '_' || ch == '-') {
            ch = ' ';
        }
    }
    bool capitalize = true;
    for (char& ch : base) {
        if (std::isspace(static_cast<unsigned char>(ch))) {
            capitalize = true;
        } else if (capitalize) {
            ch =
                static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            capitalize = false;
        } else {
            ch =
                static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
    }
    return base;
}

}  // namespace

std::vector<MediaItem> LoadMediaItems(const std::filesystem::path& directory) {
    std::vector<MediaItem> items;
    if (!std::filesystem::exists(directory)) {
        return items;
    }
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const std::string fileName = entry.path().filename().string();
        MediaItem item{};
        item.id    = fileName;
        item.label = PrettyItemName(fileName);
        item.path  = entry.path();
        items.push_back(std::move(item));
    }
    std::sort(items.begin(), items.end(),
              [](const MediaItem& a, const MediaItem& b) {
                  return ToLower(a.id) < ToLower(b.id);
              });
    return items;
}

}  // namespace sdl3cpp::services::impl
