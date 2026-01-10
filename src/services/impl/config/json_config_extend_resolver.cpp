#include "json_config_extend_resolver.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl::json_config {

namespace {
constexpr const char* kExtendsKey = "extends";
}

std::vector<std::filesystem::path> JsonConfigExtendResolver::Extract(
    const rapidjson::Value& document,
    const std::filesystem::path& configPath) const {
    std::vector<std::filesystem::path> paths;
    if (!document.HasMember(kExtendsKey)) {
        return paths;
    }

    const auto& extendsValue = document[kExtendsKey];
    auto resolvePath = [&](const std::filesystem::path& candidate) {
        if (candidate.is_absolute()) {
            return candidate;
        }
        return configPath.parent_path() / candidate;
    };

    if (extendsValue.IsString()) {
        paths.push_back(resolvePath(extendsValue.GetString()));
    } else if (extendsValue.IsArray()) {
        for (const auto& entry : extendsValue.GetArray()) {
            if (!entry.IsString()) {
                throw std::runtime_error("JSON member 'extends' must be a string or array of strings");
            }
            paths.push_back(resolvePath(entry.GetString()));
        }
    } else {
        throw std::runtime_error("JSON member 'extends' must be a string or array of strings");
    }

    return paths;
}

}  // namespace sdl3cpp::services::impl::json_config
