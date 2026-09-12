#include "services/interfaces/workflow/gta5/ped/gta5_ped.hpp"

#include "services/interfaces/workflow/gta5/resource/gta5_drawable_mesh.hpp"
#include "services/interfaces/workflow/gta5/ped/gta5_ped_frame.hpp"

#include <algorithm>
#include <limits>

namespace sdl3cpp::services::impl {
namespace {

bool Fail(const std::shared_ptr<ILogger>& logger, const std::string& why) {
    if (logger) logger->Warn("gta5.player.character: " + why);
    return false;
}

}  // namespace

bool LoadGta5Ped(Gta5StreamState& state, SDL_GPUDevice* device,
                 const Gta5PedSpec& spec, Gta5Ped& ped,
                 const std::shared_ptr<ILogger>& logger) {
    const std::string base = spec.dir + "/" + spec.name;
    Gta5Resource yft, ydd, ytd;
    if (!LoadGta5Resource(base + ".yft", yft) ||
        !ReadGta5Skeleton(yft, yft.Follow(0x30), ped.skeleton)) {
        return Fail(logger, "no skeleton in " + base + ".yft");
    }
    if (!LoadGta5Resource(base + ".ydd", ydd) ||
        !LoadGta5Resource(base + ".ytd", ytd)) {
        return Fail(logger, "cannot read " + base + ".ydd and .ytd");
    }
    for (const std::string& component : spec.components) {
        const std::int64_t drawable =
            LocateGta5Drawable(ydd, base + ".ydd", Gta5Hash(component));
        if (drawable < 0) {
            Fail(logger, "no " + component + " in " + base);
            continue;
        }
        for (Gta5PedPart& part : ReadGta5PedParts(ydd, drawable)) {
            ped.parts.push_back(std::move(part));
        }
    }
    if (ped.parts.empty()) return Fail(logger, "no parts in " + base);
    ped.geometry.subMeshes.resize(ped.parts.size());
    ped.feet = std::numeric_limits<float>::max();
    std::size_t vertices = 0;
    for (std::size_t i = 0; i < ped.parts.size(); ++i) {
        if (!UploadGta5PedPart(state, device, ytd, ped.parts[i],
                               ped.geometry.subMeshes[i], ped.textures)) {
            return Fail(logger, "arena upload failed");
        }
        for (const BspRenderVertex& v : ped.parts[i].mesh.vertices) {
            ped.feet = std::min(ped.feet, v.y);
        }
        vertices += ped.parts[i].mesh.vertices.size();
    }
    ped.geometry.usable = true;
    ped.geometry.references = 1;  // never released: the step owns it
    if (logger) {
        logger->Info("gta5.player.character: " + spec.name + ", " +
                     std::to_string(ped.parts.size()) + " parts, " +
                     std::to_string(vertices) + " vertices, " +
                     std::to_string(ped.skeleton.names.size()) + " bones, " +
                     std::to_string(ped.textures.size()) + " textured");
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
