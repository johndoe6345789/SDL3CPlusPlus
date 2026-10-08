#include "services/interfaces/workflow/switchback/session/switchback_session.hpp"

#include <array>
#include <cstddef>

namespace sdl3cpp::services::impl {
namespace {

struct DrawDistanceStep {
    float metres;
    const char* label;
};

constexpr std::array<DrawDistanceStep, kSwitchbackDrawDistanceSteps>
    kDrawDistances{{
        {1500.f, "1.5 KM"},
        {3000.f, "3 KM"},
        {4000.f, "4 KM"},
        {6000.f, "6 KM"},
    }};

}  // namespace

float SwitchbackDrawDistanceM(int step) {
    return kDrawDistances[static_cast<std::size_t>(step)].metres;
}

const char* SwitchbackDrawDistanceLabel(int step) {
    return kDrawDistances[static_cast<std::size_t>(step)].label;
}

}  // namespace sdl3cpp::services::impl
