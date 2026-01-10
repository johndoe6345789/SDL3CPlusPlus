#pragma once

#include <any>
#include <string>
#include <unordered_map>
#include <utility>

namespace sdl3cpp::services {

class WorkflowContext {
public:
    template <typename T>
    void Set(const std::string& key, T value) {
        values_[key] = std::move(value);
    }

    bool Contains(const std::string& key) const {
        return values_.find(key) != values_.end();
    }

    bool Remove(const std::string& key) {
        return values_.erase(key) > 0u;
    }

    template <typename T>
    const T* TryGet(const std::string& key) const {
        auto it = values_.find(key);
        if (it == values_.end()) {
            return nullptr;
        }
        return std::any_cast<T>(&it->second);
    }

    const std::any* TryGetAny(const std::string& key) const {
        auto it = values_.find(key);
        if (it == values_.end()) {
            return nullptr;
        }
        return &it->second;
    }

private:
    std::unordered_map<std::string, std::any> values_;
};

}  // namespace sdl3cpp::services
