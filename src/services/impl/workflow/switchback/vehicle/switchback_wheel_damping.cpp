#include "services/interfaces/workflow/switchback/vehicle/switchback_wheel_damping.hpp"

#include <string>

namespace sdl3cpp::services::impl {
namespace {

// Critical damping is 2 sqrt(k), about 14 at the gta5 stiffness of 50. The
// values sit just above it, so the body settles after a bump and does not
// ring on the springs.
constexpr float kRelaxationDamping = 17.f;
constexpr float kCompressionDamping = 15.f;

}  // namespace

void DampSwitchbackWheels(Gta5Vehicle& car) {
    if (!car.vehicle) return;
    for (int i = 0; i < car.vehicle->getNumWheels(); ++i) {
        btWheelInfo& wheel = car.vehicle->getWheelInfo(i);
        wheel.m_wheelsDampingRelaxation = kRelaxationDamping;
        wheel.m_wheelsDampingCompression = kCompressionDamping;
    }
}

std::string DescribeSwitchbackSuspension(const Gta5Vehicle& car) {
    if (!car.vehicle) return "susp=none";
    std::string text = "susp=";
    for (int i = 0; i < car.vehicle->getNumWheels(); ++i) {
        const btWheelInfo& wheel = car.vehicle->getWheelInfo(i);
        if (i > 0) text += ",";
        text += std::to_string(wheel.m_raycastInfo.m_suspensionLength);
    }
    return text;
}

}  // namespace sdl3cpp::services::impl
