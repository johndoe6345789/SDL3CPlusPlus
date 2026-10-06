#include "services/interfaces/workflow/racer/world/racer_pod_rig.hpp"

namespace sdl3cpp::services::impl {

RacerEffectShapes BuildRacerEffectShapes(SDL_GPUDevice* device,
                                         const RacerGpuTexture& white) {
    RacerEffectShapes shapes;
    shapes.flame = UploadRacerShape(device, white,
                       RacerConeVertices(12, {1.f, 0.85f, 0.55f, 0.55f},
                                         {1.f, 0.3f, 0.05f, 0.f}),
                       true);
    shapes.boostFlame = UploadRacerShape(device, white,
                            RacerConeVertices(12, {0.8f, 0.9f, 1.f, 0.7f},
                                              {0.35f, 0.55f, 1.f, 0.f}),
                            true);
    // A flat unit square on the ground, darkest at its middle.
    const RacerGpuVertex edge{0, 0, 0, 0.5f, 0.5f, 0.f, 0.f, 0, 0, 0};
    auto corner = [&](float x, float z) {
        RacerGpuVertex v = edge;
        v.x = x;
        v.z = z;
        return v;
    };
    RacerGpuVertex middle = edge;
    middle.alpha = 0.5f;
    const RacerGpuVertex c[4] = {corner(-0.5f, -0.5f), corner(0.5f, -0.5f),
                                 corner(0.5f, 0.5f), corner(-0.5f, 0.5f)};
    std::vector<RacerGpuVertex> quad;
    for (int k = 0; k < 4; ++k) {
        quad.insert(quad.end(), {c[k], c[(k + 1) % 4], middle});
    }
    shapes.shadow = UploadRacerShape(device, white, quad, true);
    return shapes;
}

}  // namespace sdl3cpp::services::impl
