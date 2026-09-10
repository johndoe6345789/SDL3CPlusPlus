#pragma once

#include <nlohmann/json.hpp>

#include <array>
#include <string>

namespace sdl3cpp::services::impl {

/// The weapons in Quake's slot order: index 0 is the gauntlet, which is
/// slot 1 on the keyboard.
const std::array<const char*, 9>& Q3WeaponOrder();

/**
 * @brief Whether a weapon can be cycled to.
 *
 * ioq3 cg_weapons.c CG_WeaponSelectable(): it has to be carried and to
 * have ammo. Selecting by number only needs the weapon itself, which is
 * why that path does not go through here.
 */
bool IsWeaponSelectable(const nlohmann::json& inventory,
                        const nlohmann::json& ammo, const std::string& weapon);

/**
 * @brief The next selectable weapon `direction` slots from `current`.
 *
 * ioq3 CG_NextWeapon_f / CG_PrevWeapon_f: walk the slots in order,
 * wrapping, and take the first that is selectable. The gauntlet is
 * skipped — Quake never cycles onto it — and `current` comes back
 * unchanged when nothing else can be selected.
 */
std::string CycleWeapon(const nlohmann::json& inventory,
                        const nlohmann::json& ammo, const std::string& current,
                        int direction);

/**
 * @brief The md3 prefix the viewmodel is loaded under for a weapon.
 *
 * Each weapon's model is read into the context under its own prefix, and
 * the viewmodel draw picks one by name, the way Quake's cg.weaponSelect
 * chooses which registered weapon to put in the player's hands.
 */
std::string Q3WeaponModelPrefix(const std::string& weapon);

}  // namespace sdl3cpp::services::impl
