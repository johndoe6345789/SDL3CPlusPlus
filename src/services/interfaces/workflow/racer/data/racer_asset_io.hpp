#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Whole-file read. Empty optional if the file cannot be opened.
std::optional<std::vector<std::uint8_t>> ReadRacerFile(
    const std::filesystem::path& path);

/// Writes RGBA8 pixels as a PNG, creating parent folders as needed.
bool WriteRacerPng(const std::filesystem::path& path, int width, int height,
                   const std::vector<std::uint8_t>& rgba);

/// Decodes a TGA (or any stb-readable image) to RGBA8.
struct RacerImage {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;
};
std::optional<RacerImage> ReadRacerImage(const std::filesystem::path& path);

/// Regular files in `dir`, recursing when asked. A missing folder yields
/// an empty list rather than an exception.
std::vector<std::filesystem::path> ListRacerFiles(
    const std::filesystem::path& dir, bool recurse);

}  // namespace sdl3cpp::services::impl
