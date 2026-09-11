#include "services/interfaces/workflow/gta5/gta5_draw_instances.hpp"

#include "services/interfaces/workflow/gta5/gta5_draw_one.hpp"
#include "services/interfaces/workflow/gta5/gta5_frustum.hpp"

namespace sdl3cpp::services::impl {

int DrawGta5Instances(const Gta5StreamState& state,
                      const Gta5DrawContext& draw) {
    if (!draw.pass || !draw.cmd) return 0;

    // Everything resident was drawn every frame, behind the camera too:
    // 19,000 draws for a desert road, which held it at 6 fps.
    const Gta5Frustum frustum = MakeGta5Frustum(draw.proj * draw.view);
    int drawn = 0;
    SDL_GPUTexture* boundTexture = nullptr;
    for (const auto& entry : state.resident) {
        for (const Gta5Instance& instance : entry.second.instances) {
            if (!Gta5InstanceVisible(frustum, instance, draw.cameraPos,
                                     draw.cullSizeRatio)) {
                continue;
            }
            drawn += DrawGta5Instance(instance, draw, boundTexture);
        }
    }
    // Vehicles are not part of any tile: they belong to the physics
    // world, not the streamer, so they draw after it.
    for (const Gta5Vehicle& car : state.vehicles) {
        drawn += DrawGta5Instance(car.instance, draw, boundTexture);
        if (!car.hasWheels) continue;
        for (const Gta5Instance& wheel : car.wheels) {
            drawn += DrawGta5Instance(wheel, draw, boundTexture);
        }
    }
    return drawn;
}

}  // namespace sdl3cpp::services::impl
