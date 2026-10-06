#include "services/interfaces/workflow/racer/data/racer_asset_export.hpp"

#include "services/interfaces/workflow/racer/data/racer_asset_library.hpp"
#include "services/interfaces/workflow/racer/data/racer_track_table.hpp"

#include <fstream>
#include <string>

namespace sdl3cpp::services::impl {
namespace {

std::string TextureName(const RacerMaterialRef& m) {
    return "tex_" + std::to_string(m.textureIndex) + "_" +
           std::to_string(m.width * (m.doubleWidth ? 2 : 1)) + "x" +
           std::to_string(m.height * (m.doubleHeight ? 2 : 1)) + ".png";
}

bool WriteObj(const RacerModel& model, const std::filesystem::path& obj) {
    std::ofstream out(obj);
    std::ofstream mtl(std::filesystem::path(obj).replace_extension(".mtl"));
    if (!out || !mtl) return false;
    out << "mtllib " << obj.stem().string() << ".mtl\n";
    std::size_t base = 1;
    for (std::size_t b = 0; b < model.batches.size(); ++b) {
        const RacerModelBatch& batch = model.batches[b];
        mtl << "newmtl m" << b << "\nKd 1 1 1\n";
        if (batch.material.textureIndex >= 0) {
            mtl << "map_Kd ../textures/" << TextureName(batch.material)
                << "\n";
        }
        out << "usemtl m" << b << "\n";
        for (const RacerModelVertex& v : batch.vertices) {
            // OBJ is y-up; the game is z-up.
            out << "v " << v.x << ' ' << v.z << ' ' << -v.y << ' '
                << v.r / 255.f << ' ' << v.g / 255.f << ' ' << v.b / 255.f
                << "\nvt " << v.u << ' ' << 1.f - v.v << "\n";
        }
        for (std::size_t i = 0; i + 2 < batch.vertices.size(); i += 3) {
            const std::size_t a = base + i;
            out << "f " << a << '/' << a << ' ' << a + 1 << '/' << a + 1
                << ' ' << a + 2 << '/' << a + 2 << "\n";
        }
        base += batch.vertices.size();
    }
    return true;
}

}  // namespace

int ExportRacerModels(const RacerExportOptions& options,
                      const std::shared_ptr<ILogger>& logger) {
    const RacerAssetLibrary library = OpenRacerAssetLibrary(options.racerDir);
    const RacerTrackTable table = LoadRacerTrackTable(options.trackTable);
    if (!library.valid || !table.loaded) return 0;
    std::filesystem::create_directories(options.outDir / "models");
    int written = 0;
    for (const RacerTrackInfo& track : table.tracks) {
        const RacerModel model = LoadRacerModel(library, track.model);
        if (logger) {
            logger->Info("racer: " + track.name + " model: " +
                         std::to_string(model.meshCount) + " meshes, " +
                         std::to_string(model.triangleCount) +
                         " triangles, " +
                         std::to_string(model.batches.size()) + " materials");
        }
        const auto path = options.outDir / "models" /
                          ("track_" + std::to_string(track.id) + ".obj");
        if (model.valid && WriteObj(model, path)) ++written;
    }
    for (std::size_t i = 0; i < table.racers.size(); ++i) {
        const RacerModel pod = LoadRacerModel(
            library, table.racers[i].podd, RacerModelScope::PodParts);
        const auto path = options.outDir / "models" /
                          ("pod_" + std::to_string(i) + ".obj");
        if (pod.valid && WriteObj(pod, path)) ++written;
    }
    return written;
}

}  // namespace sdl3cpp::services::impl
