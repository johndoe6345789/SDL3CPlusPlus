#pragma once

#include <any>
#include <cstdint>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <stdexcept>

namespace sdl3cpp::services {

class WorkflowContext {
public:
    template <typename T>
    void Set(const std::string& key, T value) {
        if constexpr (std::is_pointer_v<T> && !std::is_same_v<std::decay_t<T>, const char*>) {
            // Store non-string pointers in separate map to avoid typeid issues across TUs
            native_ptrs_[key] = const_cast<void*>(reinterpret_cast<const void*>(value));
        } else {
            values_[key] = std::move(value);
        }
    }

    bool Contains(const std::string& key) const {
        return values_.find(key) != values_.end() ||
               native_ptrs_.find(key) != native_ptrs_.end();
    }

    bool Remove(const std::string& key) {
        bool removed = values_.erase(key) > 0u;
        removed |= native_ptrs_.erase(key) > 0u;
        return removed;
    }

    template <typename T>
    const T* TryGet(const std::string& key) const {
        if constexpr (std::is_pointer_v<T> && !std::is_same_v<std::decay_t<T>, const char*>) {
            // Non-string pointers stored in native_ptrs_ to avoid typeid issues across TUs
            auto it = native_ptrs_.find(key);
            if (it == native_ptrs_.end()) {
                return nullptr;
            }
            thread_local T cached_ptr;
            cached_ptr = reinterpret_cast<T>(it->second);
            return &cached_ptr;
        } else {
            auto it = values_.find(key);
            if (it == values_.end()) {
                return nullptr;
            }
            return std::any_cast<T>(&it->second);
        }
    }

    const std::any* TryGetAny(const std::string& key) const {
        auto it = values_.find(key);
        if (it == values_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    /**
     * @brief Get a value with a default fallback. Type-safe.
     *
     * @param key The context key
     * @param defaultValue Value to return if key not found or type mismatch
     * @return The value if found and correct type, otherwise defaultValue
     */
    template <typename T>
    T Get(const std::string& key, T defaultValue = T()) const {
        const T* ptr = TryGet<T>(key);
        return ptr ? *ptr : defaultValue;
    }

    /**
     * @brief Get a value or throw if not found/wrong type.
     *
     * @param key The context key
     * @return The value
     * @throws std::runtime_error if key not found or type mismatch
     */
    template <typename T>
    const T& GetRequired(const std::string& key) const {
        const T* ptr = TryGet<T>(key);
        if (!ptr) {
            throw std::runtime_error("WorkflowContext: Required key '" + key + "' not found or type mismatch");
        }
        return *ptr;
    }

    /**
     * @brief Get an integer value (works with double/float conversion).
     *
     * @param key The context key
     * @param defaultValue Value to return if key not found
     * @return The integer value (converted from double if needed)
     */
    int GetInt(const std::string& key, int defaultValue = 0) const {
        if (auto* dval = TryGet<double>(key)) {
            return static_cast<int>(*dval);
        }
        if (auto* ival = TryGet<int>(key)) {
            return *ival;
        }
        return defaultValue;
    }

    /**
     * @brief Get a double value.
     *
     * @param key The context key
     * @param defaultValue Value to return if key not found
     * @return The double value
     */
    double GetDouble(const std::string& key, double defaultValue = 0.0) const {
        const double* ptr = TryGet<double>(key);
        return ptr ? *ptr : defaultValue;
    }

    /**
     * @brief Get a string value.
     *
     * @param key The context key
     * @param defaultValue Value to return if key not found
     * @return The string value
     */
    std::string GetString(const std::string& key, const std::string& defaultValue = "") const {
        const std::string* ptr = TryGet<std::string>(key);
        return ptr ? *ptr : defaultValue;
    }

    /**
     * @brief Get a boolean value.
     *
     * @param key The context key
     * @param defaultValue Value to return if key not found
     * @return The boolean value
     */
    bool GetBool(const std::string& key, bool defaultValue = false) const {
        const bool* ptr = TryGet<bool>(key);
        return ptr ? *ptr : defaultValue;
    }

private:
    std::unordered_map<std::string, std::any> values_;
    std::unordered_map<std::string, void*> native_ptrs_;
};

}  // namespace sdl3cpp::services
