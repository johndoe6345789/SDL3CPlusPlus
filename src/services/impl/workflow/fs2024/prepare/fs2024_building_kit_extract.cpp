#include "services/interfaces/workflow/fs2024/prepare/fs2024_building_kit_extract.hpp"

#include "services/interfaces/workflow/fs2024/prepare/fs2024_dds_texture.hpp"

#include <filesystem>
#include <stb_image_write.h>

namespace sdl3cpp::tools::fs2024 {
namespace {
namespace fs = std::filesystem;

void ExtractOne(const std::string& ddsPath, const fs::path& outPath) {
    if (fs::exists(outPath)) return;
    const auto image = DecodeDds(ddsPath);
    stbi_write_png(outPath.string().c_str(), image.width, image.height, 4,
                  image.rgba.data(), image.width * 4);
}

}  // namespace

void ExtractBuildingKit(const std::string& wallDdsPath,
                       const std::string& roofDdsPath,
                       const std::string& outDir) {
    const fs::path kitDir = fs::path(outDir) / "building_kit";
    fs::create_directories(kitDir);
    ExtractOne(wallDdsPath, kitDir / "wall.png");
    ExtractOne(roofDdsPath, kitDir / "roof.png");
}

}  // namespace sdl3cpp::tools::fs2024
