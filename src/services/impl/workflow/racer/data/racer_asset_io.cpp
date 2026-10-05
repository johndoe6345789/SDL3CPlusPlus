#include "services/interfaces/workflow/racer/data/racer_asset_io.hpp"

#include <stb_image.h>
#include <stb_image_write.h>

#include <fstream>
#include <iterator>
#include <system_error>
#include <vector>

namespace sdl3cpp::services::impl {


std::optional<std::vector<std::uint8_t>> ReadRacerFile(
    const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return std::nullopt;
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in),
                                     std::istreambuf_iterator<char>());
}

bool WriteRacerPng(const std::filesystem::path& path, int width, int height,
                   const std::vector<std::uint8_t>& rgba) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    return stbi_write_png(path.string().c_str(), width, height, 4,
                          rgba.data(), width * 4) != 0;
}

std::optional<RacerImage> ReadRacerImage(const std::filesystem::path& path) {
    int width = 0;
    int height = 0;
    int channels = 0;
    const std::string name = path.string();
    unsigned char* pixels =
        stbi_load(name.c_str(), &width, &height, &channels, 4);
    if (!pixels) return std::nullopt;
    RacerImage image;
    image.width = width;
    image.height = height;
    image.rgba.assign(pixels, pixels + static_cast<std::size_t>(width) *
                                           height * 4);
    stbi_image_free(pixels);
    return image;
}

std::vector<std::filesystem::path> ListRacerFiles(
    const std::filesystem::path& dir, bool recurse) {
    std::vector<std::filesystem::path> files;
    try {
        if (recurse) {
            for (const auto& e :
                 std::filesystem::recursive_directory_iterator(dir)) {
                if (e.is_regular_file()) files.push_back(e.path());
            }
        } else {
            for (const auto& e : std::filesystem::directory_iterator(dir)) {
                if (e.is_regular_file()) files.push_back(e.path());
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
    }
    return files;
}

}  // namespace sdl3cpp::services::impl
