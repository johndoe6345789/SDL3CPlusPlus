#pragma once

#include "services/interfaces/workflow/gta5/gta5_ped.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// GTA's axes into the engine's. The engine's (x, y, z) is GTA's
/// (x, -z, y), so GTA's up lands on the engine's and GTA's forward on
/// the engine's -z. A pose read in the one only holds in the other
/// through this and its inverse.
inline glm::mat4 Gta5ToEngine() {
    return glm::mat4(glm::vec4(1.f, 0.f, 0.f, 0.f),
                     glm::vec4(0.f, 0.f, -1.f, 0.f),
                     glm::vec4(0.f, 1.f, 0.f, 0.f),
                     glm::vec4(0.f, 0.f, 0.f, 1.f));
}

/// Where the right hand is in the ped's own space this frame: its posed
/// bone. False when the skeleton has no such bone.
bool Gta5HandMatrix(const Gta5Skeleton& skeleton,
                    const std::vector<glm::mat4>& skin, glm::mat4& hand);

/// The half turn that puts a weapon the right way round in the fist.
glm::mat4 Gta5GripTurn(const Gta5Skeleton& skeleton);

/// The gun in the hand. `want` is a weapons.rpf model (w_pi_pistol and
/// the like) held in `dir`; it is read once and kept in `geometry`,
/// with `loaded` saying which is there. The instance is added to the
/// character, so it is drawn and culled with the player.
void AddGta5HeldWeapon(Gta5StreamState& state, SDL_GPUDevice* device,
                       const Gta5Ped& ped, const std::vector<glm::mat4>& skin,
                       const glm::mat4& model, const std::string& dir,
                       const std::string& want, Gta5Geometry& geometry,
                       std::string& loaded,
                       const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
