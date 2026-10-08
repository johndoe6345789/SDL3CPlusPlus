#pragma once

namespace sdl3cpp::services::impl {

/// Six speed gearbox for the switchback car. The revs climb through a gear
/// to the redline, then the box changes up and the revs fall back.
class SwitchbackGearbox {
public:
    /// Feeds the car's speed in m/s, the magnitude of its velocity.
    void Update(float speed);

    /// The gear in use, 1 to 6.
    int Gear() const { return gear_; }
    /// Engine revs, 0 at idle to 1 at the redline.
    float Revs() const { return revs_; }

private:
    int gear_ = 1;
    float revs_ = 0.f;
};

}  // namespace sdl3cpp::services::impl
