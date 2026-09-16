#pragma once

#include "services/interfaces/workflow/gta5/hud/gta5_menu.hpp"

#include <cstdint>
#include <string>

namespace sdl3cpp::services::impl {

/// What the HUD draws this frame.
struct Gta5HudState {
    float health{100.f};
    float armour{0.f};
    std::string weapon;  // empty: nothing in hand, nothing shown
    int clip{0};         // -1: no ammunition (melee)
    int reserve{0};
    bool aiming{false};      // the reticle is up only then
    bool showWeapon{false};  // the weapon and ammunition, top right
    bool driving{false};
    float kmh{0.f};
    float revs{0.f};  // 0 idle .. 1 redline
    int gear{1};
    std::string prompt;  // "E  LS CUSTOMS": a shop's door
    bool menuOpen{false};
    Gta5Menu menu;
    Gta5Wheel wheel;
};

/// GTA shows the weapon readout while it matters -- aiming, firing,
/// choosing, a new weapon -- and for a few seconds after, not always.
struct Gta5HudWeaponTimer {
    std::string weapon;
    int clip{-1};
    std::uint64_t until{0};  // SDL ticks, ms
};

/// Whether the readout shows now, given @p state and the time.
bool Gta5HudWeaponShown(Gta5HudWeaponTimer& timer,
                        const Gta5HudState& state, std::uint64_t now);

}  // namespace sdl3cpp::services::impl
