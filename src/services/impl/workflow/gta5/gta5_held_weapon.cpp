#include "services/interfaces/workflow/gta5/gta5_held_weapon.hpp"

#include "services/interfaces/workflow/gta5/gta5_drawable_geometry.hpp"
#include "services/interfaces/workflow/gta5/gta5_drawable_mesh.hpp"
#include "services/interfaces/workflow/gta5/gta5_resource.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

/// A weapon model is a .ydr of its own, outside the map's index, so it
/// is read straight from the folder the weapons were extracted to.
bool LoadWeapon(Gta5StreamState& state, SDL_GPUDevice* device,
                const std::string& dir, const std::string& model,
                Gta5Geometry& geometry,
                const std::shared_ptr<ILogger>& logger) {
    const std::string path = dir + "/" + model + ".ydr";
    Gta5Resource res;
    if (!LoadGta5Resource(path, res)) {
        if (logger) logger->Warn("gta5.weapon: cannot read " + path);
        return false;
    }
    const std::int64_t drawable =
        LocateGta5Drawable(res, path, Gta5Hash(model));
    if (drawable < 0) return false;
    const Gta5MeshData mesh = ReadGta5DrawableMesh(res, drawable);
    return UploadGta5MeshGeometry(state, mesh, device, geometry, false,
                                  model, logger);
}

}  // namespace

bool Gta5HandMatrix(const Gta5Skeleton& skeleton,
                    const std::vector<glm::mat4>& skin, glm::mat4& hand) {
    for (std::size_t i = 0; i < skeleton.names.size(); ++i) {
        if (skeleton.names[i] != "SKEL_R_Hand") continue;
        if (i >= skin.size() || i >= skeleton.rest.size()) return false;
        // skin takes the bind pose to where the bone is now; through the
        // bone's own rest pose that is where the hand has ended up.
        hand = skin[i] * skeleton.rest[i];
        return true;
    }
    return false;
}

void AddGta5HeldWeapon(Gta5StreamState& state, SDL_GPUDevice* device,
                       const Gta5Ped& ped, const std::vector<glm::mat4>& skin,
                       const glm::mat4& model, const std::string& dir,
                       const std::string& want, Gta5Geometry& geometry,
                       std::string& loaded,
                       const std::shared_ptr<ILogger>& logger) {
    if (want.empty() || dir.empty()) return;
    if (want != loaded) {
        geometry = Gta5Geometry{};
        loaded = LoadWeapon(state, device, dir, want, geometry, logger)
                     ? want
                     : std::string();
        if (!loaded.empty() && logger) {
            logger->Info("gta5.weapon: holding " + want);
        }
    }
    glm::mat4 hand(1.f);
    if (loaded.empty() || !Gta5HandMatrix(ped.skeleton, skin, hand)) return;
    // In the fist, pointing the way the hand does.
    const glm::mat4 grip =
        glm::translate(glm::mat4(1.f), glm::vec3(0.f, -0.02f, 0.06f));
    Gta5Instance gun;
    gun.geometry = &geometry;
    const glm::mat4 placed = model * hand * grip;
    std::memcpy(gun.modelMatrix.data(), glm::value_ptr(placed),
                sizeof(float) * 16);
    state.character.push_back(gun);
}

}  // namespace sdl3cpp::services::impl
