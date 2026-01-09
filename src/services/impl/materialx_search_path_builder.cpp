#include "materialx_search_path_builder.hpp"
#include "materialx_path_resolver.hpp"

#include <filesystem>

namespace sdl3cpp::services::impl {

MaterialX::FileSearchPath MaterialXSearchPathBuilder::Build(
    const MaterialXConfig& config,
    const std::filesystem::path& scriptDirectory) const {
    MaterialX::FileSearchPath searchPath;
    MaterialXPathResolver resolver;
    std::filesystem::path libraryPath = resolver.Resolve(config.libraryPath, scriptDirectory);
    if (libraryPath.empty() && !scriptDirectory.empty()) {
        auto fallback = scriptDirectory.parent_path() / "MaterialX" / "libraries";
        if (std::filesystem::exists(fallback)) {
            libraryPath = fallback;
        }
    }
    if (!libraryPath.empty()) {
        searchPath.append(MaterialX::FilePath(libraryPath.string()));
    }
    return searchPath;
}

}  // namespace sdl3cpp::services::impl
