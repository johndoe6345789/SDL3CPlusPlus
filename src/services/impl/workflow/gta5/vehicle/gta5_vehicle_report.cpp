#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_report.hpp"

#include <cstdio>

namespace sdl3cpp::services::impl {
namespace {

std::string Num(float value) {
    char text[32];
    std::snprintf(text, sizeof(text), "%.2f", value);
    return text;
}

}  // namespace

std::string DescribeGta5Vehicle(const Gta5Vehicle& car) {
    if (!car.chassis) return "gta5.vehicle: no chassis";
    const btVector3 p = car.chassis->getWorldTransform().getOrigin();
    const btVector3 v = car.chassis->getLinearVelocity();
    std::string out = "gta5.vehicle: at (" + Num(p.x()) + "," + Num(p.y()) +
                      "," + Num(p.z()) + ") speed " + Num(v.length()) +
                      " m/s" + (car.held ? " HELD" : "");
    for (int i = 0; car.vehicle && i < car.vehicle->getNumWheels(); ++i) {
        const btWheelInfo& w = car.vehicle->getWheelInfo(i);
        out += " | w" + std::to_string(i) +
               (w.m_raycastInfo.m_isInContact ? " contact" : " air") +
               " len " + Num(w.m_raycastInfo.m_suspensionLength) +
               " engine " + Num(w.m_engineForce) + " susp " +
               Num(w.m_wheelsSuspensionForce);
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
