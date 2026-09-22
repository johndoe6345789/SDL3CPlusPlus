#pragma once

#include "services/interfaces/workflow/stunts/data/stunts_car.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Where the car is and what the drivetrain is doing.
struct StuntsCarState {
    glm::vec3 position{0.f};
    float heading = 0.f;   ///< Radians about +Y; 0 drives toward +X.
    float speed = 0.f;     ///< Metres per second, negative in reverse.
    float rpm = 0.f;
    int gear = 1;
    bool onRoad = true;
};

/// One frame of driver input, each already clamped to [-1, 1].
struct StuntsDriveInput {
    float throttle = 0.f;  ///< Positive drives, negative brakes.
    float steer = 0.f;     ///< Positive turns right.
};

/// How the model turns a car's engine block into motion.
struct StuntsDriveTuning {
    float topSpeed = 70.f;       ///< Metres per second in top gear.
    float drive = 7.f;           ///< Peak acceleration, m/s^2.
    float brake = 12.f;
    float drag = 0.4f;
    float offRoadDrag = 4.f;     ///< Extra drag once off the road.
    float steerRate = 1.4f;      ///< Radians per second at low speed.
};

/// Derives the tuning a car's own engine figures imply.
StuntsDriveTuning StuntsTuningFor(const StuntsEngine& engine);

/// Engine speed the current road speed and gear imply.
float StuntsRpmFor(const StuntsEngine& engine, const StuntsDriveTuning& tune,
                   float speed, int gear);

/// Picks the gear the box would be in at this speed.
int StuntsGearFor(const StuntsEngine& engine, const StuntsDriveTuning& tune,
                  float speed, int gear);

/// Advances `state` by `dt` seconds under `input`.
void StepStuntsCar(StuntsCarState& state, const StuntsEngine& engine,
                   const StuntsDriveTuning& tune,
                   const StuntsDriveInput& input, float dt);

}  // namespace sdl3cpp::services::impl
