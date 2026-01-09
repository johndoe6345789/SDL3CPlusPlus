#include "json_config_schema_path_resolver.hpp"

#include <system_error>
#include <vector>

namespace sdl3cpp::services::impl::json_config {

std::filesystem::path JsonConfigSchemaPathResolver::Resolve(const std::filesystem::path& configPath) const {
    const std::filesystem::path schemaFile = "runtime_config_v2.schema.json";
    std::vector<std::filesystem::path> candidates;
    if (!configPath.empty()) {
        candidates.push_back(configPath.parent_path() / "schema" / schemaFile);
    }
    candidates.push_back(std::filesystem::current_path() / "config" / "schema" / schemaFile);

    std::error_code ec;
    for (const auto& candidate : candidates) {
        if (!candidate.empty() && std::filesystem::exists(candidate, ec)) {
            return candidate;
        }
    }
    return {};
}

}  // namespace sdl3cpp::services::impl::json_config
