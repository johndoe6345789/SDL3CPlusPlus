// Weapon cycling, to ioq3's rules: the wheel walks the slots in order,
// skips the gauntlet, and only stops on a weapon you carry and have
// ammo for (cg_weapons.c CG_NextWeapon_f / CG_WeaponSelectable).

#include "services/interfaces/workflow/quake3/q3_weapon_cycle.hpp"

#include <gtest/gtest.h>

namespace impl = sdl3cpp::services::impl;

namespace {

nlohmann::json CarryAll() {
    nlohmann::json owned = nlohmann::json::object();
    for (const char* weapon : impl::Q3WeaponOrder()) {
        owned[weapon] = true;
    }
    return owned;
}

nlohmann::json AmmoForAll(int rounds) {
    nlohmann::json ammo = nlohmann::json::object();
    for (const char* weapon : impl::Q3WeaponOrder()) {
        ammo[weapon] = rounds;
    }
    return ammo;
}

}  // namespace

TEST(WeaponCycle, ForwardTakesTheNextSlot) {
    EXPECT_EQ(impl::CycleWeapon(CarryAll(), AmmoForAll(10),
                                "weapon_machinegun", 1),
              "weapon_shotgun");
}

TEST(WeaponCycle, BackwardTakesThePreviousSlot) {
    EXPECT_EQ(
        impl::CycleWeapon(CarryAll(), AmmoForAll(10), "weapon_shotgun", -1),
        "weapon_machinegun");
}

TEST(WeaponCycle, SkipsWeaponsYouDoNotCarry) {
    auto owned = CarryAll();
    owned["weapon_shotgun"]         = false;
    owned["weapon_grenadelauncher"] = false;
    EXPECT_EQ(
        impl::CycleWeapon(owned, AmmoForAll(10), "weapon_machinegun", 1),
        "weapon_rocketlauncher");
}

TEST(WeaponCycle, SkipsWeaponsYouHaveNoAmmoFor) {
    auto ammo             = AmmoForAll(10);
    ammo["weapon_shotgun"] = 0;
    EXPECT_EQ(
        impl::CycleWeapon(CarryAll(), ammo, "weapon_machinegun", 1),
        "weapon_grenadelauncher");
}

TEST(WeaponCycle, NeverCyclesOntoTheGauntlet) {
    // Quake skips it deliberately: it is slot 1, reachable by number,
    // but the wheel must not land you on a melee weapon mid-fight.
    auto ammo = AmmoForAll(0);
    ammo["weapon_machinegun"] = 50;
    ammo["weapon_gauntlet"]   = 50;
    EXPECT_EQ(impl::CycleWeapon(CarryAll(), ammo, "weapon_machinegun", 1),
              "weapon_machinegun")
        << "wrapping past the end landed on the gauntlet";
}

TEST(WeaponCycle, WrapsAroundTheEnd) {
    auto ammo             = AmmoForAll(0);
    ammo["weapon_bfg"]    = 5;
    ammo["weapon_shotgun"] = 5;
    EXPECT_EQ(impl::CycleWeapon(CarryAll(), ammo, "weapon_bfg", 1),
              "weapon_shotgun");
}

TEST(WeaponCycle, KeepsTheCurrentWeaponWhenNothingElseIsSelectable) {
    auto ammo                 = AmmoForAll(0);
    ammo["weapon_machinegun"] = 50;
    EXPECT_EQ(impl::CycleWeapon(CarryAll(), ammo, "weapon_machinegun", 1),
              "weapon_machinegun");
}

TEST(WeaponCycle, EachWeaponMapsToItsOwnViewmodel) {
    EXPECT_EQ(impl::Q3WeaponModelPrefix("weapon_machinegun"), "weapon_mg");
    EXPECT_EQ(impl::Q3WeaponModelPrefix("weapon_railgun"), "weapon_railgun");
    EXPECT_EQ(impl::Q3WeaponModelPrefix("weapon_rocketlauncher"),
              "weapon_rocketl");
    EXPECT_EQ(impl::Q3WeaponModelPrefix("something_else"), "weapon_mg")
        << "an unknown weapon should fall back to the machinegun";
}
