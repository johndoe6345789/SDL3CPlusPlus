#include "services/interfaces/workflow/gta5/hud/gta5_shop_step.hpp"

#include "services/interfaces/workflow/gta5/effects/gta5_effects_spawn.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_hold.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_seat.hpp"
#include "services/interfaces/workflow_context.hpp"

namespace sdl3cpp::services::impl {

void WorkflowGta5ShopStep::SwapCar(WorkflowContext& context,
                                   std::size_t index) {
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    if (!device || !world || index >= garage_.cars.size()) return;
    const Gta5GarageCar& pick = garage_.cars[index];
    Gta5VehicleSpec spec;
    spec.model = pick.model + "_hi";
    spec.wheel = pick.wheel;
    spec.wheelRadius = pick.radius;
    spec.paint = colour_ >= 0 ? garage_.colours[colour_].rgb
                              : glm::vec3(0.6f, 0.62f, 0.64f);
    // Out front on the nearest road, facing the way its lane runs.
    if (!roads_.loaded && !roadsDir_.empty()) LoadGta5Roads(roadsDir_, roads_);
    glm::vec3 spot = standAt_;
    Gta5RoadSpot road;
    if (NearestGta5Road(roads_, standAt_, 150.f, road)) {
        spot = road.at;
        spec.heading = glm::degrees(road.yaw);
    }
    float ground = 0.f;
    if (Gta5GroundBelow(world, spot + glm::vec3(0.f, 2.f, 0.f), ground)) {
        spot.y = ground + 1.5f;
    }
    if (logger_) logger_->Info("gta5.shop: taking the old car away");
    while (!state_->vehicles.empty()) {
        const std::size_t last = state_->vehicles.size() - 1;
        // Any bullet holes riding on it go with it: they hold its
        // chassis to follow, and it is about to be deleted.
        DropGta5MarksOn(*Gta5EffectsOf(context),
                        state_->vehicles[last].chassis);
        RemoveGta5Vehicle(*state_, last, world);
    }
    if (logger_) logger_->Info("gta5.shop: bringing " + pick.name);
    const bool ok = SpawnGta5Vehicle(*state_, spec, spot, pick.mass, device,
                                     world, logger_);
    if (logger_) {
        logger_->Info("gta5.shop: " + pick.name +
                      (ok ? " is yours" : " failed"));
    }
    state_->settings.car = pick.model;
    Remember(context);
    page_ = Gta5ShopPage::Closed;  // go and see it
}

}  // namespace sdl3cpp::services::impl
