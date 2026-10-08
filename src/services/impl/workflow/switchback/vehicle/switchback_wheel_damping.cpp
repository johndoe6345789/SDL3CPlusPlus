#include "services/interfaces/workflow/switchback/vehicle/switchback_wheel_damping.hpp"

namespace sdl3cpp::services::impl {
namespace {

// gta5 sets 6.0 and 4.4, which rings after each bump.
constexpr float kRelaxationDamping = 9.f;
constexpr float kCompressionDamping = 7.f;

}  // namespace

void DampSwitchbackWheels(Gta5Vehicle& car) {
    if (!car.vehicle) return;
    for (int i = 0; i < car.vehicle->getNumWheels(); ++i) {
        btWheelInfo& wheel = car.vehicle->getWheelInfo(i);
        wheel.m_wheelsDampingRelaxation = kRelaxationDamping;
        wheel.m_wheelsDampingCompression = kCompressionDamping;
    }
}

}  // namespace sdl3cpp::services::impl
