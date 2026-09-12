#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_geometry.hpp"
#include "services/interfaces/workflow/gta5/render/gta5_mesh_extract.hpp"
#include "services/interfaces/workflow/gta5/ped/gta5_skeleton.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

struct Gta5StreamState;

/// Copies of a skinned part's vertices: one is written each frame while
/// frames still in flight draw the others.
inline constexpr int kGta5PedRing = 3;

/// One geometry of a ped, in its bind pose, with what moves it.
struct Gta5PedPart {
    Gta5SubMeshData mesh;  // engine space, as ReadGta5Vertex reads it
    std::vector<std::array<std::uint8_t, 4>> bones;  // skeleton indices
    std::vector<std::array<float, 4>> weights;       // summing to one
    std::array<Gta5ArenaSlot, kGta5PedRing> ring{};
};

/// A ped put together: its skeleton, its parts, the geometry they draw
/// as (a submesh each), and the textures it owns.
struct Gta5Ped {
    Gta5Skeleton skeleton;
    std::vector<Gta5PedPart> parts;
    Gta5Geometry geometry;
    std::vector<SDL_GPUTexture*> textures;
    float feet{0.f};  // lowest bind-pose height: what stands on the floor
};

struct Gta5PedWalk {
    float phase{0.f};   // radians through the two-step cycle
    float amount{0.f};  // 0 standing, 1 walking; eased
    float breath{0.f};  // seconds, for the rise and fall of standing
};

/// Where a ped comes from: `dir` holds name.yft (the skeleton), name.ydd
/// (the components, by name) and name.ytd (their textures).
struct Gta5PedSpec {
    std::string dir;
    std::string name;
    std::vector<std::string> components;
};

/// A ped drawable's high-detail geometries, with their skinning: blend
/// weights in declaration slot 16, indices in slot 20, and those mapped
/// to skeleton bones through the geometry's bone ids (+0x68, count +0x72).
std::vector<Gta5PedPart> ReadGta5PedParts(const Gta5Resource& res,
                                          std::int64_t drawable);

/// Read the ped and upload its parts and textures. False, and logged,
/// when anything is missing.
bool LoadGta5Ped(Gta5StreamState& state, SDL_GPUDevice* device,
                 const Gta5PedSpec& spec, Gta5Ped& ped,
                 const std::shared_ptr<ILogger>& logger);

/// This frame's skin matrices: the legs and arms swung for a walk at
/// `speed` m/s, and the arms brought up down the sights as `aim` goes
/// to one, along a view pitched `pitch` radians (+ is up).
void PoseGta5Ped(const Gta5Skeleton& skeleton, Gta5PedWalk& walk,
                 float speed, float dt, float aim, float pitch,
                 bool twoHanded, std::vector<glm::mat4>& skin);

/// A part's bind-pose vertices moved by `skin`, weight-blended.
void SkinGta5PedPart(const Gta5PedPart& part,
                     const std::vector<glm::mat4>& skin,
                     std::vector<BspRenderVertex>& out);

}  // namespace sdl3cpp::services::impl
