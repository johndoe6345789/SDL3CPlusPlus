#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <utility>

namespace sdl3cpp::fs2024 {

/// A cache any number of loader threads may share: each value is made
/// once (outside the lock, so one slow decode never stalls the others)
/// and handed out as a shared pointer, which stays valid however the
/// cache itself is trimmed. A null value -- data FS2024 does not have
/// there, open ocean -- is remembered like any other. Two threads that
/// miss on one key at once both make it; the first to finish is kept.
template <typename Key, typename Value>
class SharedCache {
public:
    /// At most `capacity` values (0: unbounded); a full cache is
    /// emptied rather than tracked for recency -- the loaders walk the
    /// world in order, so what they need next is rarely what they had.
    explicit SharedCache(std::size_t capacity = 0) : capacity_(capacity) {}

    template <typename Make>
    std::shared_ptr<const Value> Get(const Key& key, Make&& make) {
        {
            std::lock_guard<std::mutex> hold(lock_);
            const auto found = values_.find(key);
            if (found != values_.end()) return found->second;
        }
        std::shared_ptr<const Value> made = make();
        std::lock_guard<std::mutex> hold(lock_);
        if (capacity_ != 0 && values_.size() >= capacity_) values_.clear();
        return values_.emplace(key, std::move(made)).first->second;
    }

private:
    std::mutex lock_;
    std::size_t capacity_;
    std::map<Key, std::shared_ptr<const Value>> values_;
};

}  // namespace sdl3cpp::fs2024
