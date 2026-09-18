#include "services/interfaces/workflow/fs2024/data/texture/fs2024_pgg_kit_extract.hpp"

#include "services/interfaces/workflow/fs2024/data/texture/fs2024_dds_array.hpp"
#include "services/interfaces/workflow/fs2024/data/texture/fs2024_facade_bake.hpp"

#include <filesystem>
#include <stb_image_write.h>

namespace sdl3cpp::fs2024 {
namespace {
namespace fs = std::filesystem;

/// London brick, a domestic window and a clay tile roof: the assets
/// FS2024's own British styles are built from.
constexpr const char* kWallAsset = "wall/wall_bricks_04.png";
constexpr const char* kWindowAsset = "window/w_res_8_c.png";
/// Not one of the `roof_colorize/*` assets: those are greyscale
/// masks the generator tints per building, and used as-is they make
/// a whole city of grey roofs.
constexpr const char* kRoofAsset = "roof/roof_tiles_01.png";
/// Matches the mesh's own wall tiling (fs2024_building_mesh.hpp).
constexpr float kBayMetres = 4.f;
constexpr float kStoreyMetres = 3.f;
constexpr int kPixelsPerMetre = 64;

void WritePng(const fs::path& path, const DdsImage& image) {
    stbi_write_png(path.string().c_str(), image.width, image.height, 4,
                  image.rgba.data(), image.width * 4);
}

}  // namespace

PggBuildingKit BakePggBuildingKit(const std::string& pggDir) {
    const std::string albedo = pggDir + "/TEXTURES_ALBEDO.DDS.DDS";
    const auto assets = ReadPggAssets(pggDir + "/textures.json");
    const PggAsset& wall = FindPggAsset(assets, kWallAsset);
    const PggAsset& window = FindPggAsset(assets, kWindowAsset);
    const PggAsset& roof = FindPggAsset(assets, kRoofAsset);
    PggBuildingKit kit;
    kit.wall = BakeFacadeBay(wall, ReadDdsArrayLayer(albedo, wall.albedoLayer),
                             window,
                             ReadDdsArrayLayer(albedo, window.albedoLayer),
                             kBayMetres, kStoreyMetres, kPixelsPerMetre);
    kit.roof = ReadDdsArrayLayer(albedo, roof.albedoLayer);
    return kit;
}

void ExtractPggBuildingKit(const std::string& pggDir,
                          const std::string& outDir) {
    const fs::path kitDir = fs::path(outDir) / "building_kit";
    fs::create_directories(kitDir);
    const PggBuildingKit kit = BakePggBuildingKit(pggDir);
    WritePng(kitDir / "wall.png", kit.wall);
    WritePng(kitDir / "roof.png", kit.roof);
}

}  // namespace sdl3cpp::fs2024
