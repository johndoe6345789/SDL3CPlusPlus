#include "services/interfaces/workflow/gta5/gta5_held_weapon.hpp"

#include "services/interfaces/workflow/gta5/gta5_drawable_geometry.hpp"
#include "services/interfaces/workflow/gta5/gta5_drawable_mesh.hpp"
#include "services/interfaces/workflow/gta5/gta5_resource.hpp"
#include "services/interfaces/workflow/gta5/gta5_weapon_dress.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

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
    if (!UploadGta5MeshGeometry(state, mesh, device, geometry, false, model,
                                logger)) {
        return false;
    }
    DressGta5Weapon(state, device, dir, model, mesh, geometry, logger);
    return true;
}

}  // namespace

void AddGta5HeldWeapon(Gta5StreamState& state, SDL_GPUDevice* device,
                       const Gta5Ped& ped, const std::vector<glm::mat4>& skin,
                       const glm::mat4& model, const std::string& dir,
                       const std::string& want, Gta5Geometry& geometry,
                       std::string& loaded,
                       const std::shared_ptr<ILogger>& logger) {
    if (want.empty() || dir.empty()) return;
    if (want != loaded) {
        geometry = Gta5Geometry{};
        const bool got = LoadWeapon(state, device, dir, want, geometry,
                                   logger);
        loaded = got ? want : std::string();
        if (!loaded.empty() && logger) {
            logger->Info("gta5.weapon: holding " + want);
        }
    }
    glm::mat4 hand(1.f);
    if (loaded.empty() || !Gta5HandMatrix(ped.skeleton, skin, hand)) return;
    // In the fist: laid along the hand, then nudged into the palm.
    const glm::mat4 grip =
        glm::translate(glm::mat4(1.f), glm::vec3(0.f, -0.02f, 0.02f)) *
        Gta5GripTurn(ped.skeleton);
    Gta5Instance gun;
    gun.geometry = &geometry;
    const glm::mat4 placed = model * hand * grip;
    std::memcpy(gun.modelMatrix.data(), glm::value_ptr(placed),
                sizeof(float) * 16);
    state.character.push_back(gun);
}

}  // namespace sdl3cpp::services::impl
