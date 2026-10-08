#pragma once

namespace sdl3cpp::services::impl {

/// Eight speed gearbox for the switchback car. The engine revs up under
/// throttle and falls back to the wheels off it. The box changes up at the
/// redline and the revs drop to the wheel speed in the new gear. At the
/// redline in top gear the rev limiter cuts the engine's power.
class SwitchbackGearbox {
public:
    /// Feeds the car's speed in m/s (its magnitude), the throttle from 0 to
    /// 1, and the time since the last update in seconds.
    void Update(float speed, float throttle, float dt);

    /// The gear in use, 1 to 8.
    int Gear() const { return gear_; }
    /// Engine revs, 0 at idle to 1 at the redline.
    float Revs() const { return revs_; }
    /// Share of the engine's power allowed through: fades to 0 at the rev
    /// limiter.
    float PowerScale() const { return powerScale_; }

private:
    int gear_ = 1;
    float revs_ = 0.f;
    float powerScale_ = 1.f;
    // Engine revs when not held by the wheels: the clutch slips above them.
    float freeRevs_ = 0.f;
};

}  // namespace sdl3cpp::services::impl
