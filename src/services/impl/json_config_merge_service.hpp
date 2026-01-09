#pragma once

#include <memory>
#include <string>

#include <rapidjson/document.h>

namespace sdl3cpp::services {
class ILogger;
}

namespace sdl3cpp::services::impl::json_config {

class JsonConfigMergeService {
public:
    explicit JsonConfigMergeService(std::shared_ptr<ILogger> logger);

    void Merge(rapidjson::Value& target,
               const rapidjson::Value& overlay,
               rapidjson::Document::AllocatorType& allocator,
               const std::string& jsonPath) const;

private:
    void ApplyDeleteDirectives(rapidjson::Value& target,
                               const rapidjson::Value& overlay,
                               const std::string& jsonPath) const;

    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl::json_config
