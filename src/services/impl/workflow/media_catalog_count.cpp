#include "services/interfaces/workflow/media_catalog_builder.hpp"

namespace sdl3cpp::services::impl {

std::size_t CountMediaCatalogItems(const MediaCatalog& catalog) {
    std::size_t itemCount = 0;
    for (const auto& category : catalog.categories) {
        itemCount += category.items.size();
    }
    return itemCount;
}

}  // namespace sdl3cpp::services::impl
