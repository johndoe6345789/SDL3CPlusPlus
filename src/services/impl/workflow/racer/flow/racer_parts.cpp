#include "services/interfaces/workflow/racer/flow/racer_parts.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

// The game's parts table: name, price, podiums before Watto stocks it.
const RacerPartInfo kParts[kRacerUpgradeCount][kRacerUpgradeMax + 1] = {
    {{"R-20 Repulsorgrip", 250, 0}, {"R-60 Repulsorgrip", 400, 0},
     {"R-80 Repulsorgrip", 600, 2}, {"R-100 Repulsorgrip", 1200, 6},
     {"R-300 Repulsorgrip", 2600, 10}, {"R-600 Repulsorgrip", 6000, 14}},
    {{"Control Linkage", 200, 0}, {"Control Shift Plate", 400, 0},
     {"Control Vectro-Jet", 700, 2}, {"Control Coupling", 1600, 6},
     {"Control Nozzle", 3800, 12}, {"Control Stabilizer", 7500, 16}},
    {{"Dual 20PcX Injector", 800, 0}, {"44 PcX Injector", 2200, 0},
     {"Dual32PcX Injector", 5600, 4}, {"Quad 32PcX Injector", 7000, 8},
     {"Quad 44 Injector", 10400, 10}, {"Mag-6 Injector", 14000, 12}},
    {{"Plug2 Thrust Coil", 1000, 0}, {"Plug3 Thrust Coil", 2400, 0},
     {"Plug5 Thrust Coil", 6000, 2}, {"Plug8 Thrust Coil", 14000, 8},
     {"Block5 Thrust Coil", 17500, 12}, {"Block6 Thrust Coil", 20000, 16}},
    {{"Mark II Air Brake", 700, 0}, {"Mark III Air Brake", 1400, 0},
     {"Mark IV Air Brake", 3600, 4}, {"Mark V Air Brake", 7000, 6},
     {"Tri-jet Air Brake", 10400, 10}, {"Quadrijet Air Brake", 14000, 14}},
    {{"Coolant Radiator", 50, 0}, {"Stack-3 Radiator", 100, 0},
     {"Stack-6 Radiator", 300, 6}, {"Rod Coolant Pump", 900, 8},
     {"Dual Coolant Pump", 2700, 10}, {"Turbo Coolant Pump", 5400, 14}},
    {{"Single Power Cell", 150, 0}, {"Dual Power Cell", 300, 0},
     {"Quad Power Cell", 800, 2}, {"Cluster Power Plug", 1400, 4},
     {"Rotary Power Plug", 4000, 8}, {"Cluster2 Power Plug", 7000, 12}}};

}  // namespace

const RacerPartInfo& RacerPart(int type, int level) {
    type = std::clamp(type, 0, kRacerUpgradeCount - 1);
    level = std::clamp(level, 0, kRacerUpgradeMax);
    return kParts[type][level];
}

const char* RacerUpgradeName(int type) {
    static const char* const kNames[kRacerUpgradeCount] = {
        "TRACTION", "TURNING", "ACCELERATION", "TOP SPEED",
        "AIR BRAKE", "COOLING", "REPAIR"};
    return type >= 0 && type < kRacerUpgradeCount ? kNames[type] : "";
}

int RacerTradeInValue(int type, int level, float health) {
    const auto price = static_cast<float>(RacerPart(type, level).price);
    const float value = 0.25f * price;
    return static_cast<int>(value * std::clamp(health, 0.f, 1.f));
}

}  // namespace sdl3cpp::services::impl
