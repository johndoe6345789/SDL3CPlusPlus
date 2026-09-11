#pragma once

#include "services/interfaces/workflow/gta5/gta5_weapons.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// What the player keeps between sessions: the car they drive and its
/// colour, what they carry, and how well they are protected.
struct Gta5Settings {
    std::string car;     // a garage model, empty for the game's own
    int colour{-1};      // an index into the garage's colours
    float health{100.f};
    float armour{0.f};
    Gta5Inventory inventory;
    std::string station;  // the radio station last tuned in
};

/// Where they are kept: %APPDATA%\SDL3CPlusPlus\gta5\settings.json on
/// Windows, $XDG_CONFIG_HOME (or ~/.config) elsewhere. Empty when the
/// environment says nothing, in which case nothing is saved.
std::string Gta5SettingsPath();

/// Reads them, leaving the defaults where the file says nothing; false
/// when there is no file yet.
bool LoadGta5Settings(Gta5Settings& settings);

/// Writes them, making the folder if it is not there.
bool SaveGta5Settings(const Gta5Settings& settings);

}  // namespace sdl3cpp::services::impl
