#pragma once

#include "services/interfaces/workflow/gta5/gta5_map_art.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace sdl3cpp::services::impl {

/// The atlas' cells: what a piece of an effect looks like.
enum Gta5Sprite { kGta5Puff = 0, kGta5Flash, kGta5Smoke, kGta5Scorch };

/// One piece of an effect: facing the camera, lying on a surface
/// (`normal`), or drawn along one (`length`, a tracer).
struct Gta5Particle {
    glm::vec3 at{0.f};
    glm::vec3 velocity{0.f};
    glm::vec3 normal{0.f};
    glm::vec3 colour{1.f};
    float size{1.f};
    float growth{0.f};   // metres a second
    float length{0.f};   // a streak this long along `normal`
    float gravity{0.f};  // metres a second squared, downward
    float drag{0.f};     // a share of its speed a second
    float age{0.f};
    float life{1.f};
    float fade{1.f};  // how bright it starts
    int sprite{kGta5Puff};
};

/// Everything alight, and the art it draws with; shared through the
/// context (gta5.effects).
struct Gta5Effects {
    std::vector<Gta5Particle> particles;
    SDL_GPUTexture* atlas{nullptr};
    SDL_GPUSampler* sampler{nullptr};
    bool ready{false};
};
using Gta5EffectsPtr = std::shared_ptr<Gta5Effects>;

/// The shared list, made on first use.
Gta5EffectsPtr Gta5EffectsOf(WorkflowContext& context);

/// The four sprites, drawn rather than loaded.
bool CreateGta5EffectAtlas(Gta5Effects& effects, SDL_GPUDevice* device,
                           Gta5UploadBatch& uploads);

/// A shot leaving the barrel: flash and smoke.
void SpawnGta5Muzzle(Gta5Effects& effects, const glm::vec3& at,
                     const glm::vec3& ahead);

/// The round's streak, barrel to strike.
void SpawnGta5Tracer(Gta5Effects& effects, const glm::vec3& from,
                     const glm::vec3& to);

/// Where a round struck: dust, sparks, a hole.
void SpawnGta5Impact(Gta5Effects& effects, const glm::vec3& at,
                     const glm::vec3& normal);

/// A rocket going off: flash, fireball, climbing smoke.
void SpawnGta5Explosion(Gta5Effects& effects, const glm::vec3& at,
                        float radius);
/// The mark left behind, which stays a while and fades.
void SpawnGta5Scorch(Gta5Effects& effects, const glm::vec3& at,
                     const glm::vec3& normal, float radius, float seconds);

/// Carry them forward `dt` seconds and drop the spent ones.
void UpdateGta5Effects(Gta5Effects& effects, float dt);

/// This frame's quads: colour in the normal, brightness in lm_u.
std::vector<BspRenderVertex> BuildGta5EffectQuads(const Gta5Effects& effects,
                                                  const glm::vec3& right,
                                                  const glm::vec3& up);

}  // namespace sdl3cpp::services::impl
