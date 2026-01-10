#pragma once

#include <filesystem>

namespace sdl3cpp::services::impl {

class MaterialXPathResolver {
public:
    std::filesystem::path Resolve(const std::filesystem::path& path,
                                  const std::filesystem::path& scriptDirectory) const;
};

}  // namespace sdl3cpp::services::impl
