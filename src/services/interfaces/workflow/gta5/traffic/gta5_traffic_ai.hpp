#pragma once

#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_types.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

constexpr int kGta5DriverInputs = 8;
constexpr int kGta5DriverOutputs = 3;

/// What one driver knows, in metres and seconds. The baseline, the
/// logger and the network all read exactly this and nothing else, so a
/// network trained on it can be swapped in for the baseline unchanged.
struct Gta5DriverSense {
    float speed{0.f};    // m/s along the ground
    float off{0.f};      // radians the aim point is off its nose
    float turning{0.f};  // rad/s about the vertical
    float gap{1000.f};   // metres to whatever is in front
    float line{1000.f};  // metres to a red light's line, else 1000
    float want{9.f};     // personality: the speed it likes, m/s
    float follow{6.f};   // personality: the gap it stops behind, m
    float nerve{1.f};    // personality: how hard it takes bends
};

/// What a driver does with the controls.
struct Gta5DriverAction {
    float steer{0.f};     // -1..1
    float throttle{0.f};  // 0..1
    float brake{0.f};     // 0..1
};

/// The nearest thing in front of a car: other traffic, and the player's
/// own cars. 1000 when the road is clear.
float Gta5TrafficGapAhead(const Gta5Traffic& traffic,
                          const Gta5StreamState& state,
                          const Gta5TrafficCar& self, const glm::vec3& at,
                          float facing);

/// Fill in everything but speed, off and turning, which the caller has.
Gta5DriverSense Gta5SenseDriver(const Gta5Traffic& traffic,
                                const Gta5StreamState& state,
                                const Gta5Roads& roads,
                                const Gta5TrafficCar& car,
                                const glm::vec3& at, float facing);

/// Sense scaled to roughly -1..1 for a network. Also the layout the
/// training script reads, so change both together.
void Gta5DriverInputs(const Gta5DriverSense& sense, float* out);

/// The hand-written driver: pure of the sense vector.
Gta5DriverAction Gta5BaselineDriver(const Gta5DriverSense& sense);

/// A small fully connected net: ReLU hidden layers, tanh steer,
/// sigmoid throttle and brake. Loaded from the file the trainer wrote.
class Gta5DriverNet {
public:
    bool Load(const std::string& path);
    bool Ready() const { return !layers_.empty(); }
    Gta5DriverAction Run(const Gta5DriverSense& sense) const;

private:
    struct Layer {
        int in{0};
        int out{0};
        std::vector<float> weights;  // out rows of in
        std::vector<float> bias;
    };
    std::vector<Layer> layers_;
};

/// Baseline, or the net named by SDL3CPP_TRAFFIC_POLICY when set. The
/// baseline's answer is appended to SDL3CPP_TRAFFIC_LOG when that is
/// set, whichever of the two drives, so a net's own mistakes get
/// labelled too.
Gta5DriverAction Gta5DecideDrive(const Gta5DriverSense& sense);

}  // namespace sdl3cpp::services::impl
