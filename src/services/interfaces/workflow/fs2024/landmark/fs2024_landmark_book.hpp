#pragma once

#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_mesh.hpp"

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace sdl3cpp::services::impl {

/// What the loader threads know of each landmark model, shared with the
/// main thread: its bounds once any thread has decoded it, and whether
/// its kit is on the GPU now -- so a tile standing a model already
/// uploaded clears its buildings from the bounds and skips decoding it
/// again. Every call is safe from any thread.
class Fs2024LandmarkBook {
public:
    void Record(const std::string& name, const Fs2024LandmarkBounds& bounds);
    void SetOnGpu(const std::string& name, bool onGpu);

    /// The model's bounds when its kit is on the GPU, else nothing.
    std::optional<Fs2024LandmarkBounds> Uploaded(
        const std::string& name) const;

private:
    struct Entry {
        Fs2024LandmarkBounds bounds;
        bool onGpu = false;
    };
    mutable std::mutex lock_;
    std::unordered_map<std::string, Entry> entries_;
};

}  // namespace sdl3cpp::services::impl
