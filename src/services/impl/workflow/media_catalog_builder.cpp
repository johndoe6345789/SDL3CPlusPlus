#include "services/interfaces/workflow/media_catalog_builder.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

MediaCatalog BuildMediaCatalog(const rapidjson::Document& document,
                               const std::filesystem::path& packageRoot) {
    if (!document.HasMember("categories") ||
        !document["categories"].IsArray()) {
        throw std::runtime_error("media catalog requires a categories array");
    }
    MediaCatalog catalog{};
    catalog.catalogRoot    = packageRoot;
    const auto& categories = document["categories"];
    catalog.categories.reserve(categories.Size());
    for (const auto& categoryValue : categories.GetArray()) {
        if (!categoryValue.IsObject()) {
            throw std::runtime_error(
                "media catalog categories must be objects");
        }
        if (!categoryValue.HasMember("id") || !categoryValue["id"].IsString()) {
            throw std::runtime_error(
                "media catalog category requires string id");
        }
        if (!categoryValue.HasMember("name") ||
            !categoryValue["name"].IsString()) {
            throw std::runtime_error(
                "media catalog category requires string name");
        }
        if (!categoryValue.HasMember("path") ||
            !categoryValue["path"].IsString()) {
            throw std::runtime_error(
                "media catalog category requires string path");
        }
        MediaCategory category{};
        category.id       = categoryValue["id"].GetString();
        category.name     = categoryValue["name"].GetString();
        category.basePath = packageRoot / categoryValue["path"].GetString();
        category.items    = LoadMediaItems(category.basePath);
        catalog.categories.push_back(std::move(category));
    }
    return catalog;
}

}  // namespace sdl3cpp::services::impl
