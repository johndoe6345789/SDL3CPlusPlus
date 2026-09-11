#include "services/interfaces/workflow/gta5/gta5_draw_instances.hpp"

#include "services/interfaces/workflow/gta5/gta5_draw_one.hpp"

namespace sdl3cpp::services::impl {

int DrawGta5Instances(const Gta5StreamState& state,
                      const Gta5DrawContext& draw) {
    if (!draw.pass || !draw.cmd) return 0;

    int drawn = 0;
    SDL_GPUTexture* boundTexture = nullptr;
    for (const auto& entry : state.resident) {
        for (const Gta5Instance& instance : entry.second.instances) {
            drawn += DrawGta5Instance(instance, draw, boundTexture);
        }
    }
    // Vehicles are not part of any tile: they belong to the physics
    // world, not the streamer, so they draw after it.
    for (const Gta5Instance& vehicle : state.vehicles) {
        drawn += DrawGta5Instance(vehicle, draw, boundTexture);
    }
    return drawn;
}

}  // namespace sdl3cpp::services::impl
