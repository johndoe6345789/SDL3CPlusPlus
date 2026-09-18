#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_book.hpp"

namespace sdl3cpp::services::impl {

void Fs2024LandmarkBook::Record(const std::string& name,
                                const Fs2024LandmarkBounds& bounds) {
    std::lock_guard<std::mutex> hold(lock_);
    entries_[name].bounds = bounds;
}

void Fs2024LandmarkBook::SetOnGpu(const std::string& name, bool onGpu) {
    std::lock_guard<std::mutex> hold(lock_);
    entries_[name].onGpu = onGpu;
}

std::optional<Fs2024LandmarkBounds> Fs2024LandmarkBook::Uploaded(
    const std::string& name) const {
    std::lock_guard<std::mutex> hold(lock_);
    const auto found = entries_.find(name);
    if (found == entries_.end() || !found->second.onGpu) return std::nullopt;
    return found->second.bounds;
}

}  // namespace sdl3cpp::services::impl
