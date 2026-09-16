#include "services/interfaces/workflow/gta5/hud/gta5_hud_state.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr std::uint64_t kLingerMs = 3000;

}  // namespace

bool Gta5HudWeaponShown(Gta5HudWeaponTimer& timer,
                        const Gta5HudState& state, std::uint64_t now) {
    if (state.weapon.empty()) {
        timer = Gta5HudWeaponTimer{};
        return false;
    }
    const bool changed =
        state.weapon != timer.weapon || state.clip != timer.clip;
    if (changed || state.aiming || state.wheel.open) {
        timer.until = now + kLingerMs;
    }
    timer.weapon = state.weapon;
    timer.clip   = state.clip;
    return now < timer.until;
}

}  // namespace sdl3cpp::services::impl
