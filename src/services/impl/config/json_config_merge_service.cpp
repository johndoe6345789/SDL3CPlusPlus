#include "json_config_merge_service.hpp"
#include "services/interfaces/i_logger.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl::json_config {

namespace {
constexpr const char* kDeleteKey = "@delete";
}

JsonConfigMergeService::JsonConfigMergeService(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

void JsonConfigMergeService::ApplyDeleteDirectives(rapidjson::Value& target,
                                                   const rapidjson::Value& overlay,
                                                   const std::string& jsonPath) const {
    if (!overlay.HasMember(kDeleteKey)) {
        return;
    }
    const auto& deletes = overlay[kDeleteKey];
    if (!deletes.IsArray()) {
        throw std::runtime_error("JSON member '" + std::string(kDeleteKey) + "' must be an array of strings");
    }
    for (const auto& entry : deletes.GetArray()) {
        if (!entry.IsString()) {
            throw std::runtime_error("JSON member '" + std::string(kDeleteKey) + "' must contain only strings");
        }
        const char* key = entry.GetString();
        if (target.HasMember(key)) {
            target.RemoveMember(key);
            if (logger_) {
                logger_->Trace("JsonConfigService", "ApplyDeleteDirectives",
                               "jsonPath=" + jsonPath + ", key=" + std::string(key),
                               "Removed key from merged config");
            }
        }
    }
}

void JsonConfigMergeService::Merge(rapidjson::Value& target,
                                   const rapidjson::Value& overlay,
                                   rapidjson::Document::AllocatorType& allocator,
                                   const std::string& jsonPath) const {
    if (!overlay.IsObject()) {
        target.CopyFrom(overlay, allocator);
        return;
    }

    if (!target.IsObject()) {
        target.CopyFrom(overlay, allocator);
        return;
    }

    ApplyDeleteDirectives(target, overlay, jsonPath);

    for (auto it = overlay.MemberBegin(); it != overlay.MemberEnd(); ++it) {
        const std::string memberName = it->name.GetString();
        if (memberName == kDeleteKey) {
            continue;
        }
        if (target.HasMember(memberName.c_str())) {
            auto& targetValue = target[memberName.c_str()];
            const auto& overlayValue = it->value;
            if (targetValue.IsObject() && overlayValue.IsObject()) {
                Merge(targetValue, overlayValue, allocator, jsonPath + "/" + memberName);
            } else {
                targetValue.CopyFrom(overlayValue, allocator);
            }
        } else {
            rapidjson::Value nameValue(memberName.c_str(), allocator);
            rapidjson::Value valueCopy;
            valueCopy.CopyFrom(it->value, allocator);
            target.AddMember(nameValue, valueCopy, allocator);
        }
    }
}

}  // namespace sdl3cpp::services::impl::json_config
