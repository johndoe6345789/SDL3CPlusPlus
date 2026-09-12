#pragma once

#include "services/interfaces/workflow/gta5/gta5_map_art.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <memory>
#include <vector>

namespace sdl3cpp::services::impl {

/// The atlas cells.
enum Gta5Sprite { kGta5Puff = 0, kGta5Flash, kGta5Smoke, kGta5Scorch };

/// One piece: facing the camera, on a surface (`normal`), or along one
/// (`length`, a tracer).
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
    int cell{-1};  // a cell of GTA's decal sheet, or -1 for the strip
};

/// Everything alight, shared through the context (gta5.effects).
struct Gta5Effects {
    std::vector<Gta5Particle> particles;
    SDL_GPUTexture* atlas{nullptr};
    /// GTA's own fxdecal sheet: bullet marks, 8 across and 4 down.
    SDL_GPUTexture* decals{nullptr};
    SDL_GPUSampler* sampler{nullptr};
    SDL_GPUBuffer* vertices{nullptr};
    SDL_GPUTransferBuffer* staging{nullptr};
    std::uint32_t count{0};  // strip vertices staged this frame
    std::uint32_t marks{0};  // sheet vertices, staged after those
    bool ready{false};
};
using Gta5EffectsPtr = std::shared_ptr<Gta5Effects>;

/// The shared list, made on first use.
Gta5EffectsPtr Gta5EffectsOf(WorkflowContext& context);

/// The four sprites.
bool CreateGta5EffectAtlas(Gta5Effects& effects, SDL_GPUDevice* device,
                           Gta5UploadBatch& uploads);

/// GTA's own decal sheet, from a loose fxdecal.ytd. Without one the
/// marks fall back to the drawn strip.
bool LoadGta5EffectDecals(Gta5Effects& effects, SDL_GPUDevice* device,
                          Gta5UploadBatch& uploads, const std::string& file);

/// The sprites and the buffer the quads are staged through.
void SetUpGta5Effects(Gta5Effects& effects, Gta5StreamState& state,
                      SDL_GPUDevice* device, std::uint32_t maxVertices,
                      const std::string& decalFile);

/// This frame's quads: colour in the normal, brightness in lm_u.
std::vector<BspRenderVertex> BuildGta5EffectQuads(const Gta5Effects& effects,
                                                  const glm::vec3& right,
                                                  const glm::vec3& up,
                                                  std::uint32_t& marks);

}  // namespace sdl3cpp::services::impl
