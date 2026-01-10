#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace sdl3cpp::services {

struct SoundboardClip {
    std::string id;
    std::string label;
    std::filesystem::path path;
};

struct SoundboardCategory {
    std::string id;
    std::string name;
    std::filesystem::path basePath;
    std::vector<SoundboardClip> clips;
};

struct SoundboardCatalog {
    std::filesystem::path packageRoot;
    std::vector<SoundboardCategory> categories;
};

struct SoundboardSelection {
    bool hasSelection = false;
    std::uint64_t requestId = 0;
    std::string categoryId;
    std::string clipId;
    std::string label;
    std::filesystem::path path;
};

}  // namespace sdl3cpp::services
