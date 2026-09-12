#include "services/interfaces/workflow/gta5/player/gta5_player_hold_step.hpp"

#include "services/interfaces/workflow/gta5/player/gta5_look.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_hold.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_seat.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_timer.h>

#include <string>

namespace sdl3cpp::services::impl {

bool WorkflowGta5PlayerHoldStep::Travel(WorkflowContext& context,
                                        btRigidBody* player) {
    const int sequence = context.Get<int>("gta5.player.teleport_seq", 0);
    if (sequence == travelSequence_) return false;
    travelSequence_ = sequence;
    const auto to =
        context.Get<glm::vec2>("gta5.player.teleport", glm::vec2(0.f));
    // Out of the car first; the car stays where it was.
    if (state_->seated >= 0 &&
        state_->seated < static_cast<int>(state_->vehicles.size())) {
        LeaveGta5Vehicle(state_->vehicles[state_->seated], player);
        state_->seated = -1;
    }
    // Held above any roof or peak (Chiliad is 800 m) until the ground
    // there streams in, then dropped onto whatever is highest.
    hold_ = glm::vec3(to.x, 1000.f, to.y);
    startMs_ = SDL_GetTicks();
    recorded_ = true;
    released_ = false;
    travelling_ = true;
    if (logger_) {
        logger_->Info("gta5.player.hold: travelling to (" +
                      std::to_string(static_cast<int>(to.x)) + ", " +
                      std::to_string(static_cast<int>(-to.y)) + ")");
    }
    return true;
}

void WorkflowGta5PlayerHoldStep::Arrive(WorkflowContext& context,
                                        btRigidBody* player) {
    travelling_ = false;
    if (state_->vehicles.empty()) return;
    if (!roads_.loaded && !roadsDir_.empty()) LoadGta5Roads(roadsDir_, roads_);
    const btVector3& b = player->getWorldTransform().getOrigin();
    const glm::vec3 me(b.x(), b.y(), b.z());
    // On the nearest road, in its lane and facing its way, clear of the
    // player; with none near, 5 m to the player's right.
    Gta5RoadSpot road;
    const bool onRoad = NearestGta5Road(roads_, me, 150.f, road);
    const float yaw = context.Get<float>("camera_yaw", 0.f);
    glm::vec3 spot = me + Gta5LookRight(yaw) * 5.f;
    if (onRoad) {
        spot = road.at;
        const glm::vec2 gap(spot.x - me.x, spot.z - me.z);
        if (glm::length(gap) < 4.f) {
            spot += glm::vec3(road.ahead.x, 0.f, road.ahead.y) * 7.f;
        }
    }
    // From just above the road itself: an overpass must not catch it.
    float ground = 0.f;
    if (Gta5GroundBelow(
            context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr),
            spot + glm::vec3(0.f, 2.f, 0.f), ground)) {
        spot.y = ground + 1.5f;
    }
    if (onRoad) {
        MoveGta5Vehicle(state_->vehicles.front(), spot, road.yaw);
    } else {
        MoveGta5Vehicle(state_->vehicles.front(), spot);
    }
    if (logger_) {
        logger_->Info(std::string("gta5.player.hold: car parked ") +
                      (onRoad ? "on the nearest road" : "beside the player"));
    }
}

}  // namespace sdl3cpp::services::impl
