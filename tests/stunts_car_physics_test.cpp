#include "services/interfaces/workflow/stunts/player/stunts_car_physics.hpp"

#include <gtest/gtest.h>

using sdl3cpp::services::impl::StepStuntsCar;
using sdl3cpp::services::impl::StuntsCarState;
using sdl3cpp::services::impl::StuntsDriveInput;
using sdl3cpp::services::impl::StuntsEngine;
using sdl3cpp::services::impl::StuntsGearFor;
using sdl3cpp::services::impl::StuntsRpmFor;
using sdl3cpp::services::impl::StuntsTuningFor;

namespace {

/// Shaped like the block a stock car's .RES carries.
StuntsEngine MakeEngine() {
    StuntsEngine engine;
    engine.gears = 5;
    engine.performance = 31;
    engine.idleRpm = 800;
    engine.powerRpm = 4200;
    engine.redLine = 8000;
    engine.revLimit = 8400;
    return engine;
}

void Drive(StuntsCarState& car, const StuntsEngine& engine, float seconds) {
    const auto tune = StuntsTuningFor(engine);
    const StuntsDriveInput flat{1.f, 0.f};
    for (float t = 0.f; t < seconds; t += 1.f / 60.f) {
        StepStuntsCar(car, engine, tune, flat, 1.f / 60.f);
    }
}

}  // namespace

TEST(StuntsCarPhysics, IdlesAtTheCarsOwnIdleSpeed) {
    const auto engine = MakeEngine();
    const auto tune = StuntsTuningFor(engine);
    EXPECT_FLOAT_EQ(StuntsRpmFor(engine, tune, 0.f, 1),
                    static_cast<float>(engine.idleRpm));
}

TEST(StuntsCarPhysics, NeverRevsPastTheLimit) {
    const auto engine = MakeEngine();
    const auto tune = StuntsTuningFor(engine);
    EXPECT_LE(StuntsRpmFor(engine, tune, 1000.f, 5),
              static_cast<float>(engine.revLimit));
}

TEST(StuntsCarPhysics, ClimbsThroughTheGears) {
    const auto engine = MakeEngine();
    StuntsCarState car;
    Drive(car, engine, 20.f);
    EXPECT_GT(car.gear, 1);
    EXPECT_LE(car.gear, engine.gears);
    EXPECT_GT(car.speed, 20.f);
}

TEST(StuntsCarPhysics, HoldsBelowTheCarsTopSpeed) {
    const auto engine = MakeEngine();
    const auto tune = StuntsTuningFor(engine);
    StuntsCarState car;
    Drive(car, engine, 120.f);
    EXPECT_LE(car.speed, tune.topSpeed + 0.001f);
}

TEST(StuntsCarPhysics, LeavingTheRoadCostsSpeed) {
    const auto engine = MakeEngine();
    StuntsCarState paved;
    Drive(paved, engine, 15.f);
    StuntsCarState rough;
    rough.onRoad = false;
    const auto tune = StuntsTuningFor(engine);
    const StuntsDriveInput flat{1.f, 0.f};
    for (int step = 0; step < 900; ++step) {
        rough.onRoad = false;
        StepStuntsCar(rough, engine, tune, flat, 1.f / 60.f);
    }
    EXPECT_LT(rough.speed, paved.speed);
}

TEST(StuntsCarPhysics, StationaryCarCanStillSteer) {
    // Arcade handling, matching the original: full lock is available
    // at any speed, including standstill. A car that cannot turn
    // until it is already rolling reads as broken, not as grip.
    const auto engine = MakeEngine();
    const auto tune = StuntsTuningFor(engine);
    StuntsCarState car;
    const StuntsDriveInput turning{0.f, 1.f};
    StepStuntsCar(car, engine, tune, turning, 1.f / 60.f);
    EXPECT_GT(car.heading, 0.f);
}

TEST(StuntsCarPhysics, GearSelectionStaysInRange) {
    const auto engine = MakeEngine();
    const auto tune = StuntsTuningFor(engine);
    EXPECT_EQ(StuntsGearFor(engine, tune, 0.f, 3), 1);
    EXPECT_LE(StuntsGearFor(engine, tune, 500.f, 5), engine.gears);
}
